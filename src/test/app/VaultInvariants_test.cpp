#include <test/jtx.h>
#include <test/jtx/AMM.h>
#include <test/jtx/Env.h>

#include <xrpld/app/tx/apply.h>
#include <xrpld/app/tx/detail/ApplyContext.h>

#include <xrpl/beast/unit_test/suite.h>
#include <xrpl/beast/utility/Journal.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/MPTIssue.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/STNumber.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/TxFormats.h>
#include <xrpl/protocol/XRPAmount.h>

#include <boost/algorithm/string/predicate.hpp>

#include <initializer_list>
#include <string>

namespace xrpl {
namespace test {

class VaultInvariants_test : public beast::unit_test::suite
{
    // The optional Preclose function is used to process additional transactions
    // on the ledger after creating two accounts, but before closing it, and
    // before the Precheck function. These should only be valid functions, and
    // not direct manipulations. Preclose is not commonly used.
    using Preclose = std::function<bool(
        test::jtx::Account const& a,
        test::jtx::Account const& b,
        test::jtx::Env& env)>;

    // this is common setup/method for running a failing invariant check. The
    // precheck function is used to manipulate the ApplyContext with view
    // changes that will cause the check to fail.
    using Precheck = std::function<bool(
        test::jtx::Account const& a,
        test::jtx::Account const& b,
        ApplyContext& ac)>;

    struct AccountAmount
    {
        AccountID account;
        int amount;
    };

    struct Adjustments
    {
        std::optional<int> assetsTotal = {};
        std::optional<int> assetsAvailable = {};
        std::optional<int> lossUnrealized = {};
        std::optional<int> assetsMaximum = {};
        std::optional<int> sharesTotal = {};
        std::optional<int> vaultAssets = {};
        std::optional<AccountAmount> accountAssets = {};
        std::optional<AccountAmount> accountShares = {};
    };

    enum class Asset { XRP, MPT, IOU };

    std::vector<Asset> assetTypes = {Asset::XRP, Asset::MPT, Asset::IOU};

    auto static constexpr args =
        [](AccountID id, int adjustment, auto fn) -> Adjustments {
        Adjustments sample = {
            .assetsTotal = adjustment,
            .assetsAvailable = adjustment,
            .lossUnrealized = 0,
            .assetsMaximum = adjustment,
            .sharesTotal = adjustment,
            .vaultAssets = adjustment,
            .accountAssets =  //
            AccountAmount{id, -adjustment},
            .accountShares =  //
            AccountAmount{id, adjustment},
        };
        fn(sample);
        return sample;
    };

    /** Run a specific test case to put the ledger into a state that will be
     * detected by an invariant. Simulates the actions of a transaction that
     * would violate an invariant.
     *
     * @param expect_logs One or more messages related to the failing invariant
     *  that should be in the log output
     * @precheck See "Precheck" above
     * @fee If provided, the fee amount paid by the simulated transaction.
     * @tx A mock transaction that took the actions to trigger the invariant. In
     *  most cases, only the type matters.
     * @ters The TER results expected on the two passes of the invariant
     *  checker.
     * @preclose See "Preclose" above. Note that @preclose runs *before*
     * @precheck, but is the last parameter for historical reasons
     * @setTxAccount optionally set to add sfAccount to tx (either A1 or A2)
     */
    enum class TxAccount : int { None = 0, A1, A2 };
    void
    doInvariantCheck(
        std::vector<std::string> const& expect_logs,
        Precheck const& precheck,
        XRPAmount fee = XRPAmount{},
        STTx tx = STTx{ttACCOUNT_SET, [](STObject&) {}},
        std::initializer_list<TER> ters =
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED},
        Preclose const& preclose = {},
        TxAccount setTxAccount = TxAccount::None)
    {
        using namespace test::jtx;
        FeatureBitset amendments = testable_amendments() |
            featureInvariantsV1_1 | featureSingleAssetVault;
        Env env{*this, amendments};

        Account const A1{"A1"};
        Account const A2{"A2"};
        env.fund(XRP(1000), A1, A2);
        if (preclose)
            BEAST_EXPECT(preclose(A1, A2, env));
        env.close();

        OpenView ov{*env.current()};
        test::StreamSink sink{beast::severities::kWarning};
        beast::Journal jlog{sink};
        if (setTxAccount != TxAccount::None)
            tx.setAccountID(
                sfAccount, setTxAccount == TxAccount::A1 ? A1.id() : A2.id());
        ApplyContext ac{
            env.app(),
            ov,
            tx,
            tesSUCCESS,
            env.current()->fees().base,
            tapNONE,
            jlog};

        BEAST_EXPECT(precheck(A1, A2, ac));

        // invoke check twice to cover tec and tef cases
        if (!BEAST_EXPECT(ters.size() == 2))
            return;

        TER terActual = tesSUCCESS;
        for (TER const& terExpect : ters)
        {
            terActual = ac.checkInvariants(terActual, fee);
            BEAST_EXPECTS(
            terExpect == terActual,
            "expected code: " + transToken(terExpect) +
            " actual code: " + transToken(terActual));
            auto const messages = sink.messages().str();
            BEAST_EXPECT(
                messages.starts_with("Invariant failed:") ||
                messages.starts_with("Transaction caused an exception"));
            std::cerr << messages << '\n';
            for (auto const& m : expect_logs)
            {
                if (messages.find(m) == std::string::npos)
                {
                    // uncomment if you want to log the invariant failure
                    std::cerr << "   --> " << m << std::endl;
                    fail();
                }
            }
        }
    }

    bool
    adjustBalance(
        ApplyContext& ac,
        std::optional<AccountID const> sender,
        AccountID const& receiver,
        STAmount const& amount)
    {
        if (amount.native())
        {
            auto sleAccount = ac.view().peek(keylet::account(receiver));
            if (!BEAST_EXPECT(sleAccount))
                return false;
            (*sleAccount)[sfBalance] = *(*sleAccount)[sfBalance] + amount;
            ac.view().update(sleAccount);
            return true;
        }

        if (!BEAST_EXPECT(sender))
            return false;

        return accountSend(
                   ac.view(),
                   *sender,
                   receiver,
                   amount,
                   ac.journal,
                   WaiveTransferFee::Yes) == tesSUCCESS;
    }

    bool
    adjust(ApplyContext& ac, xrpl::Keylet keylet, Adjustments args)
    {
        auto sleVault = ac.view().peek(keylet);
        if (!BEAST_EXPECT(sleVault))
            return false;

        auto const mptIssuanceID = (*sleVault)[sfShareMPTID];
        auto sleShares = ac.view().peek(keylet::mptIssuance(mptIssuanceID));
        if (!BEAST_EXPECT(sleShares))
            return false;

        // These two fields are adjusted in absolute terms
        if (args.lossUnrealized)
            (*sleVault)[sfLossUnrealized] = *args.lossUnrealized;
        if (args.assetsMaximum)
            (*sleVault)[sfAssetsMaximum] = *args.assetsMaximum;

        // Remaining fields are adjusted in terms of difference
        if (args.assetsTotal)
            (*sleVault)[sfAssetsTotal] =
                *(*sleVault)[sfAssetsTotal] + *args.assetsTotal;
        if (args.assetsAvailable)
            (*sleVault)[sfAssetsAvailable] =
                *(*sleVault)[sfAssetsAvailable] + *args.assetsAvailable;
        ac.view().update(sleVault);

        if (args.sharesTotal)
        {
            (*sleShares)[sfOutstandingAmount] =
                *(*sleShares)[sfOutstandingAmount] + *args.sharesTotal;
            ac.view().update(sleShares);
        }

        auto const assets = *(*sleVault)[sfAsset];
        auto const pseudoId = *(*sleVault)[sfAccount];

        if (args.vaultAssets)
        {
            std::optional<AccountID> sender = std::nullopt;
            AccountID receiver = pseudoId;
            STAmount amount = STAmount{assets, *args.vaultAssets};

            if (!assets.native())
            {
                sender = *args.vaultAssets > 0 ? assets.getIssuer() : pseudoId;
                receiver =
                    *args.vaultAssets > 0 ? pseudoId : assets.getIssuer();
                amount = STAmount{assets, std::abs(*args.vaultAssets)};
            }

            if (!BEAST_EXPECT(adjustBalance(ac, sender, receiver, amount)))
                return false;
        }

        if (args.accountAssets)
        {
            std::cout << "Adjusting account assets: " << to_string(assets)
                      << " by " << args.accountAssets->amount << std::endl;
            auto const& pair = *args.accountAssets;
            std::optional<AccountID> sender = std::nullopt;
            AccountID receiver = pair.account;
            STAmount amount = STAmount{assets, pair.amount};

            if (!assets.native())
            {
                sender = pair.amount > 0 ? assets.getIssuer() : pair.account;
                receiver = pair.amount > 0 ? pair.account : assets.getIssuer();
                amount = STAmount{assets, std::abs(pair.amount)};
            }

            if (!BEAST_EXPECT(adjustBalance(ac, sender, receiver, amount)))
                return false;
        }

        if (args.accountShares)
        {
            auto const& pair = *args.accountShares;
            auto sleMPToken =
                ac.view().peek(keylet::mptoken(mptIssuanceID, pair.account));
            if (!BEAST_EXPECT(sleMPToken))
                return false;
            (*sleMPToken)[sfMPTAmount] =
                *(*sleMPToken)[sfMPTAmount] + pair.amount;
            ac.view().update(sleMPToken);
        }

        return true;
    }

    void
    testVaultDeposit()
    {
        using namespace test::jtx;

        auto const vaultKeylet =
            [&](ApplyContext& ac, Account const& acc) -> std::optional<Keylet> {
            auto const accSle = ac.view().peek(keylet::account(acc.id()));
            if (!BEAST_EXPECT(accSle))
                return std::nullopt;

            auto const keylet =
                keylet::vault(acc.id(), accSle->getFieldU32(sfSequence) - 2);

            return keylet;
        };

        for (auto const assetType : assetTypes)
        {
            std::cout << "assetType: " << static_cast<int>(assetType)
                      << std::endl;

            PrettyAsset vaultAsset{xrpIssue()};

            Preclose createVault = [&](Account const& alice,
                                       Account const& bob,
                                       Env& env) -> bool {
                Account issuer{"issuer"};
                env.fund(XRP(1000), issuer);
                env.close();
            std::cout << "first hre" << std::endl;
                PrettyAsset const asset = [&]() {
                    switch (assetType)
                    {
                        case Asset::IOU: {
                            PrettyAsset const iouAsset = issuer["IOU"];
                            env(trust(alice, iouAsset(100'000'000)));
                            env(trust(bob, iouAsset(100'000'000)));
                            env(pay(issuer, alice, iouAsset(100'000'000)));
                            env(pay(issuer, bob, iouAsset(100'000'000)));
                            env.close();
                            return iouAsset;
                        }

                        case Asset::MPT: {
                            MPTTester mptt{env, issuer, mptInitNoFund};
                            mptt.create(
                                {.flags = tfMPTCanClawback | tfMPTCanTransfer |
                                     tfMPTCanLock});
                            PrettyAsset const mptAsset = mptt.issuanceID();
                            mptt.authorize({.account = alice});
                            mptt.authorize({.account = bob});
                            env(pay(issuer, alice, mptAsset(100'000'000)));
                            env(pay(issuer, bob, mptAsset(100'000'000)));
                            env.close();
                            return mptAsset;
                        }

                        case Asset::XRP:
                        default:
                            return PrettyAsset{xrpIssue()};
                    }
                }();

                // this is nasty, but becase MPT creation requires env,
                // I don't see another way to export asset to the main loop
                vaultAsset = asset;

                Vault vault{env};
                auto const [tx, keylet] =
                    vault.create({.owner = alice, .asset = asset});
                env(tx, ter(tesSUCCESS));
                env.close();

                BEAST_EXPECT(env.le(keylet));

                auto const depositVaultFunds = [&](Account const& acc,
                                                   STAmount const& amt) {
                    env(vault.deposit({
                            .depositor = acc,
                            .id = keylet.key,
                            .amount = amt,
                        }),
                        ter(tesSUCCESS));
                    env.close();
                };

                auto const amount = assetType == Asset::IOU ? asset(
                                                                  Number{
                                                                      1'234'567,
                                                                      -4,
                                                                  })
                                                            : asset(
                                                                  Number{
                                                                      1'234'567,
                                                                  });
                depositVaultFunds(alice, amount);
                depositVaultFunds(bob, amount);

                return true;
            };

            doInvariantCheck(
                {
                    "deposit must increase vault balance",
                    "deposit must decrease depositor balance",
                    "deposit must change vault and depositor balance by equal "
                    "amount",
                    "deposit and assets outstanding must add up",
                    "deposit and assets available must add up",
                },
                [&](Account const& alice,
                    Account const& bob,
                    ApplyContext& ac) {
                    auto const keylet = vaultKeylet(ac, alice);
                    if (!BEAST_EXPECT(keylet))
                        return false;

                    // Move 10 drops from A2 to A3 to enforce total XRP
                    if (!BEAST_EXPECT(adjustBalance(
                            ac,
                            vaultAsset.raw().getIssuer(),
                            bob,
                            vaultAsset(Number{10, 2}))))
                        return false;

                    return adjust(
                        ac,
                        *keylet,
                        args(bob.id(), 10, [&](Adjustments& sample) {
                            sample.vaultAssets = -20;
                            sample.accountAssets->amount = 10;
                        }));
                },
                XRPAmount{},
                STTx{
                    ttVAULT_DEPOSIT,
                    [&](STObject& tx) {

            std::cout << "next here hre" << std::endl;
                        tx[sfAmount] = vaultAsset(11);
                    }},
                {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
                createVault,
                TxAccount::A2);
            continue;

            doInvariantCheck(
                {"deposit must change vault balance"},
                [&](Account const& alice, Account const&, ApplyContext& ac) {
                    auto const keylet = vaultKeylet(ac, alice);
                    if (!BEAST_EXPECT(keylet))
                        return false;
                    return adjust(
                        ac,
                        *keylet,
                        args(alice.id(), 0, [](Adjustments& sample) {
                            sample.vaultAssets.reset();
                        }));
                },
                XRPAmount{},
                STTx{ttVAULT_DEPOSIT, [](STObject&) {}},
                {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
                createVault);

            doInvariantCheck(
                {"deposit assets outstanding must not exceed assets maximum"},
                [&](Account const& alice,
                    Account const& bob,
                    ApplyContext& ac) {
                    auto const keylet = vaultKeylet(ac, alice);
                    if (!BEAST_EXPECT(keylet))
                        return false;

                    return adjust(
                        ac,
                        *keylet,
                        args(bob.id(), 200, [&](Adjustments& sample) {
                            sample.assetsMaximum = 1;
                        }));
                },
                XRPAmount{},
                STTx{
                    ttVAULT_DEPOSIT,
                    [&](STObject& tx) {
                        tx.setFieldAmount(sfAmount, vaultAsset(200));
                    }},
                {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
                createVault,
                TxAccount::A2);

            doInvariantCheck(
                {"deposit must change depositor shares"},
                [&](Account const& alice,
                    Account const& bob,
                    ApplyContext& ac) {
                    auto const keylet = vaultKeylet(ac, alice);
                    if (!BEAST_EXPECT(keylet))
                        return false;

                    return adjust(
                        ac,
                        *keylet,
                        args(bob.id(), 10, [&](Adjustments& sample) {
                            sample.accountShares.reset();
                        }));
                },
                XRPAmount{},
                STTx{
                    ttVAULT_DEPOSIT,
                    [&](STObject& tx) { tx[sfAmount] = vaultAsset(10); }},
                {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
                createVault,
                TxAccount::A2);

            doInvariantCheck(
                {"deposit must change vault shares"},
                [&](Account const& alice,
                    Account const& bob,
                    ApplyContext& ac) {
                    auto const keylet = vaultKeylet(ac, alice);
                    if (!BEAST_EXPECT(keylet))
                        return false;

                    return adjust(
                        ac,
                        *keylet,
                        args(bob.id(), 10, [](Adjustments& sample) {
                            sample.sharesTotal = 0;
                        }));
                },
                XRPAmount{},
                STTx{
                    ttVAULT_DEPOSIT,
                    [&](STObject& tx) { tx[sfAmount] = vaultAsset(10); }},
                {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
                createVault,
                TxAccount::A2);

            doInvariantCheck(
                {"deposit must increase depositor shares",
                 "deposit must change depositor and vault shares by equal "
                 "amount",
                 "deposit must not change vault balance by more than deposited "
                 "amount"},
                [&](Account const& alice,
                    Account const& bob,
                    ApplyContext& ac) {
                    auto const keylet = vaultKeylet(ac, alice);
                    if (!BEAST_EXPECT(keylet))
                        return false;

                    return adjust(
                        ac,
                        *keylet,
                        args(bob.id(), 10, [&](Adjustments& sample) {
                            sample.accountShares->amount = -5;
                            sample.sharesTotal = -10;
                        }));
                },
                XRPAmount{},
                STTx{
                    ttVAULT_DEPOSIT,
                    [&](STObject& tx) { tx[sfAmount] = vaultAsset(5); }},
                {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
                createVault,
                TxAccount::A2);

            doInvariantCheck(
                {"deposit and assets outstanding must add up",
                 "deposit and assets available must add up"},
                [&](Account const& alice,
                    Account const& bob,
                    ApplyContext& ac) {
                    auto const keylet = vaultKeylet(ac, alice);
                    if (!BEAST_EXPECT(keylet))
                        return false;

                    return adjust(
                        ac,
                        *keylet,
                        args(bob.id(), 10, [&](Adjustments& sample) {
                            sample.assetsTotal = 7;
                            sample.assetsAvailable = 7;
                        }));
                },
                XRPAmount{},
                STTx{
                    ttVAULT_DEPOSIT,
                    [&](STObject& tx) { tx[sfAmount] = vaultAsset(10); }},
                {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
                createVault,
                TxAccount::A2);
        }
    }
    // void
    // testVault()
    // {
    //     using namespace test::jtx;

    //     Account A3{"A3"};
    //     Account A4{"A4"};

    //     auto const precloseXrp =
    //         [&](Account const& A1, Account const& A2, Env& env) -> bool {
    //         env.fund(XRP(1000), A3, A4);
    //         Vault vault{env};
    //         auto [tx, keylet] =
    //             vault.create({.owner = A1, .asset = xrpIssue()});
    //         env(tx);
    //         env(vault.deposit(
    //             {.depositor = A1, .id = keylet.key, .amount = XRP(10)}));
    //         env(vault.deposit(
    //             {.depositor = A2, .id = keylet.key, .amount = XRP(10)}));
    //         env(vault.deposit(
    //             {.depositor = A3, .id = keylet.key, .amount = XRP(10)}));
    //         return true;
    //     };

    //     testcase << "Vault deposit";

    // // This really convoluted unit tests makes the zero balance on the
    // // depositor, by sending them the same amount as the transaction
    // // fee. The operation makes no sense, but the defensive check in
    // // ValidVault::finalize is otherwise impossible to trigger.
    // doInvariantCheck(
    //     {"deposit must increase vault balance",
    //      "deposit must change depositor balance"},
    //     [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //         auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //
    //         // Move 10 drops to A4 to enforce total XRP balance
    //         auto sleA4 = ac.view().peek(keylet::account(A4.id()));
    //         if (!sleA4)
    //             return false;
    //         (*sleA4)[sfBalance] = *(*sleA4)[sfBalance] + 10;
    //         ac.view().update(sleA4);
    //
    //         return adjust(
    //             ac,
    //             keylet,
    //             args(A3.id(), -10, [&](Adjustments& sample) {
    //                 sample.accountAssets->amount = -100;
    //             }));
    //     },
    //     XRPAmount{100},
    //     STTx{
    //         ttVAULT_DEPOSIT,
    //         [&](STObject& tx) {
    //             tx[sfFee] = XRPAmount(100);
    //             tx[sfAccount] = A3.id();
    //         }},
    //     {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //     precloseXrp);

    //     doInvariantCheck(
    //         {"deposit must change depositor balance"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());

    //             // Move 10 drops from A3 to vault to enforce total XRP
    //             // balance
    //             auto sleA3 = ac.view().peek(keylet::account(A3.id()));
    //             if (!sleA3)
    //                 return false;
    //             (*sleA3)[sfBalance] = *(*sleA3)[sfBalance] - 10;
    //             ac.view().update(sleA3);

    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), 10, [&](Adjustments& sample) {
    //                     sample.accountAssets->amount = 0;
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{
    //             ttVAULT_DEPOSIT,
    //             [](STObject& tx) { tx[sfAmount] = XRPAmount(10); }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     doInvariantCheck(
    //         {"deposit and assets outstanding must add up"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto sleA3 = ac.view().peek(keylet::account(A3.id()));
    //             (*sleA3)[sfBalance] = *(*sleA3)[sfBalance] - 2000;
    //             ac.view().update(sleA3);

    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), 10, [&](Adjustments& sample) {
    //                     sample.assetsTotal = 11;
    //                 }));
    //         },
    //         XRPAmount{2000},
    //         STTx{
    //             ttVAULT_DEPOSIT,
    //             [&](STObject& tx) {
    //                 tx[sfAmount] = XRPAmount(10);
    //                 tx[sfDelegate] = A3.id();
    //                 tx[sfFee] = XRPAmount(2000);
    //             }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     testcase << "Vault withdrawal";
    //     doInvariantCheck(
    //         {"withdrawal must change vault balance"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), 0, [](Adjustments& sample) {
    //                     sample.vaultAssets.reset();
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{ttVAULT_WITHDRAW, [](STObject&) {}},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp);

    //     // Almost identical to the really convoluted test for deposit, where
    //     // the depositor spends only the transaction fee. In case of
    //     // withdrawal, this test is almost the same as normal withdrawal
    //     // where the sfDestination would have been A4, but has been omitted.
    //     doInvariantCheck(
    //         {"withdrawal must change one destination balance"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());

    //             // Move 10 drops to A4 to enforce total XRP balance
    //             auto sleA4 = ac.view().peek(keylet::account(A4.id()));
    //             if (!sleA4)
    //                 return false;
    //             (*sleA4)[sfBalance] = *(*sleA4)[sfBalance] + 10;
    //             ac.view().update(sleA4);

    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A3.id(), -10, [&](Adjustments& sample) {
    //                     sample.accountAssets->amount = -100;
    //                 }));
    //         },
    //         XRPAmount{100},
    //         STTx{
    //             ttVAULT_WITHDRAW,
    //             [&](STObject& tx) {
    //                 tx[sfFee] = XRPAmount(100);
    //                 tx[sfAccount] = A3.id();
    //                 // This commented out line causes the invariant
    //                 // violation. tx[sfDestination] = A4.id();
    //             }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp);

    //     doInvariantCheck(
    //         {"withdrawal must change vault and destination balance by "
    //          "equal amount",
    //          "withdrawal must decrease vault balance",
    //          "withdrawal must increase destination balance",
    //          "withdrawal and assets outstanding must add up",
    //          "withdrawal and assets available must add up"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());

    //             // Move 10 drops from A2 to A3 to enforce total XRP balance
    //             auto sleA3 = ac.view().peek(keylet::account(A3.id()));
    //             if (!sleA3)
    //                 return false;
    //             (*sleA3)[sfBalance] = *(*sleA3)[sfBalance] + 10;
    //             ac.view().update(sleA3);

    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), -10, [&](Adjustments& sample) {
    //                     sample.vaultAssets = 10;
    //                     sample.accountAssets->amount = -20;
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{ttVAULT_WITHDRAW, [](STObject&) {}},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     doInvariantCheck(
    //         {"withdrawal must change one destination balance"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             if (!adjust(
    //                     ac,
    //                     keylet,
    //                     args(A2.id(), -10, [&](Adjustments& sample) {
    //                         *sample.vaultAssets -= 5;
    //                     })))
    //                 return false;
    //             auto sleA3 = ac.view().peek(keylet::account(A3.id()));
    //             if (!sleA3)
    //                 return false;
    //             (*sleA3)[sfBalance] = *(*sleA3)[sfBalance] + 5;
    //             ac.view().update(sleA3);
    //             return true;
    //         },
    //         XRPAmount{},
    //         STTx{
    //             ttVAULT_WITHDRAW,
    //             [&](STObject& tx) { tx.setAccountID(sfDestination, A3.id());
    //             }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     doInvariantCheck(
    //         {"withdrawal must change depositor shares"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), -10, [&](Adjustments& sample) {
    //                     sample.accountShares.reset();
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{ttVAULT_WITHDRAW, [](STObject&) {}},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     doInvariantCheck(
    //         {"withdrawal must change vault shares"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), -10, [](Adjustments& sample) {
    //                     sample.sharesTotal = 0;
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{ttVAULT_WITHDRAW, [](STObject&) {}},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     doInvariantCheck(
    //         {"withdrawal must decrease depositor shares",
    //          "withdrawal must change depositor and vault shares by equal "
    //          "amount"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), -10, [&](Adjustments& sample) {
    //                     sample.accountShares->amount = 5;
    //                     sample.sharesTotal = 10;
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{ttVAULT_WITHDRAW, [](STObject&) {}},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     doInvariantCheck(
    //         {"withdrawal and assets outstanding must add up",
    //          "withdrawal and assets available must add up"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), -10, [&](Adjustments& sample) {
    //                     sample.assetsTotal = -15;
    //                     sample.assetsAvailable = -15;
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{ttVAULT_WITHDRAW, [](STObject&) {}},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     doInvariantCheck(
    //         {"withdrawal and assets outstanding must add up"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto sleA3 = ac.view().peek(keylet::account(A3.id()));
    //             (*sleA3)[sfBalance] = *(*sleA3)[sfBalance] - 2000;
    //             ac.view().update(sleA3);

    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), -10, [&](Adjustments& sample) {
    //                     sample.assetsTotal = -7;
    //                 }));
    //         },
    //         XRPAmount{2000},
    //         STTx{
    //             ttVAULT_WITHDRAW,
    //             [&](STObject& tx) {
    //                 tx[sfAmount] = XRPAmount(10);
    //                 tx[sfDelegate] = A3.id();
    //                 tx[sfFee] = XRPAmount(2000);
    //             }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp,
    //         TxAccount::A2);

    //     auto const precloseMpt =
    //         [&](Account const& A1, Account const& A2, Env& env) -> bool {
    //         env.fund(XRP(1000), A3, A4);

    //         // Create MPT asset
    //         {
    //             Json::Value jv;
    //             jv[sfAccount] = A3.human();
    //             jv[sfTransactionType] = jss::MPTokenIssuanceCreate;
    //             jv[sfFlags] = tfMPTCanTransfer;
    //             env(jv);
    //             env.close();
    //         }

    //         auto const mptID = makeMptID(env.seq(A3) - 1, A3);
    //         Asset asset = MPTIssue(mptID);
    //         // Authorize A1 A2 A4
    //         {
    //             Json::Value jv;
    //             jv[sfAccount] = A1.human();
    //             jv[sfTransactionType] = jss::MPTokenAuthorize;
    //             jv[sfMPTokenIssuanceID] = to_string(mptID);
    //             env(jv);
    //             jv[sfAccount] = A2.human();
    //             env(jv);
    //             jv[sfAccount] = A4.human();
    //             env(jv);

    //             env.close();
    //         }
    //         // Send tokens to A1 A2 A4
    //         {
    //             env(pay(A3, A1, asset(1000)));
    //             env(pay(A3, A2, asset(1000)));
    //             env(pay(A3, A4, asset(1000)));
    //             env.close();
    //         }

    //         Vault vault{env};
    //         auto [tx, keylet] = vault.create({.owner = A1, .asset = asset});
    //         env(tx);
    //         env(vault.deposit(
    //             {.depositor = A1, .id = keylet.key, .amount = asset(10)}));
    //         env(vault.deposit(
    //             {.depositor = A2, .id = keylet.key, .amount = asset(10)}));
    //         env(vault.deposit(
    //             {.depositor = A4, .id = keylet.key, .amount = asset(10)}));
    //         return true;
    //     };

    //     doInvariantCheck(
    //         {"withdrawal must decrease depositor shares",
    //          "withdrawal must change depositor and vault shares by equal "
    //          "amount"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), -10, [&](Adjustments& sample) {
    //                     sample.accountShares->amount = 5;
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{
    //             ttVAULT_WITHDRAW,
    //             [&](STObject& tx) { tx[sfAccount] = A3.id(); }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt,
    //         TxAccount::A2);

    //     testcase << "Vault clawback";
    //     doInvariantCheck(
    //         {"clawback must change vault balance"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac.view(),
    //                 keylet,
    //                 args(A2.id(), -1, [&](Adjustments& sample) {
    //                     sample.vaultAssets.reset();
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{
    //             ttVAULT_CLAWBACK,
    //             [&](STObject& tx) { tx[sfAccount] = A3.id(); }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);

    //     // Not the same as below check: attempt to clawback XRP
    //     doInvariantCheck(
    //         {"clawback may only be performed by the asset issuer"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac.view(),
    //                 keylet,
    //                 args(A2.id(), 0, [&](Adjustments& sample) {}));
    //         },
    //         XRPAmount{},
    //         STTx{ttVAULT_CLAWBACK, [](STObject&) {}},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseXrp);

    //     // Not the same as above check: attempt to clawback MPT by bad
    //     // account
    //     doInvariantCheck(
    //         {"clawback may only be performed by the asset issuer"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac.view(),
    //                 keylet,
    //                 args(A2.id(), 0, [&](Adjustments& sample) {}));
    //         },
    //         XRPAmount{},
    //         STTx{
    //             ttVAULT_CLAWBACK,
    //             [&](STObject& tx) { tx[sfAccount] = A4.id(); }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);

    //     doInvariantCheck(
    //         {"clawback must decrease vault balance",
    //          "clawback must decrease holder shares",
    //          "clawback must change vault shares"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac.view(),
    //                 keylet,
    //                 args(A4.id(), 10, [&](Adjustments& sample) {
    //                     sample.sharesTotal = 0;
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{
    //             ttVAULT_CLAWBACK,
    //             [&](STObject& tx) {
    //                 tx[sfAccount] = A3.id();
    //                 tx[sfHolder] = A4.id();
    //             }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);

    //     doInvariantCheck(
    //         {"clawback must change holder shares"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac.view(),
    //                 keylet,
    //                 args(A4.id(), -10, [&](Adjustments& sample) {
    //                     sample.accountShares.reset();
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{
    //             ttVAULT_CLAWBACK,
    //             [&](STObject& tx) {
    //                 tx[sfAccount] = A3.id();
    //                 tx[sfHolder] = A4.id();
    //             }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);

    //     doInvariantCheck(
    //         {"clawback must change holder and vault shares by equal amount",
    //          "clawback and assets outstanding must add up",
    //          "clawback and assets available must add up"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac.view(),
    //                 keylet,
    //                 args(A4.id(), -10, [&](Adjustments& sample) {
    //                     sample.accountShares->amount = -8;
    //                     sample.assetsTotal = -7;
    //                     sample.assetsAvailable = -7;
    //                 }));
    //         },
    //         XRPAmount{},
    //         STTx{
    //             ttVAULT_CLAWBACK,
    //             [&](STObject& tx) {
    //                 tx[sfAccount] = A3.id();
    //                 tx[sfHolder] = A4.id();
    //             }},
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);
    // }

    /*
     *
     *     void
    testVaultGeneralChecks()
    {
        testcase << "Vault general checks";
        using namespace jtx;

        auto const precloseXrp =
            [&](Account const& A1, Account const& A2, Env& env) -> bool {
            Vault vault{env};
            auto [tx, keylet] =
                vault.create({.owner = A1, .asset = xrpIssue()});
            env(tx);
            env(vault.deposit(
                {.depositor = A1, .id = keylet.key, .amount = XRP(10)}));
            env(vault.deposit(
                {.depositor = A2, .id = keylet.key, .amount = XRP(10)}));
            return true;
        };

        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_DEPOSIT, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_WITHDRAW, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CLAWBACK, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"vault operation succeeded without updating shares",
             "assets available must not be greater than assets outstanding"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                (*sleVault)[sfAssetsTotal] = 9;
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_WITHDRAW, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                env(vault.deposit(
                    {.depositor = A1, .id = keylet.key, .amount = XRP(10)}));
                return true;
            });

        doInvariantCheck(
            {"loss unrealized must not exceed the difference "
             "between assets outstanding and available",
             "vault transaction must not change loss unrealized"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                return adjust(
                    ac.view(),
                    keylet,
                    args(
                        A2.id(),
                        100,
                        [&](Adjustments& sample) {
                            sample.lossUnrealized = 13;
                        }),
                    ac.journal);
            },
            XRPAmount{},
            STTx{
                ttVAULT_DEPOSIT,
                [](STObject& tx) {
                    tx.setFieldAmount(sfAmount, XRPAmount(200));
                }},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);

        doInvariantCheck(
            {"updated shares must not exceed maximum"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*sleVault)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                (*sleShares)[sfMaximumAmount] = 10;
                ac.view().update(sleShares);

                return adjust(
                    ac.view(),
                    keylet,
                    args(A2.id(), 10, [](Adjustments&) {}),
                    ac.journal);
            },
            XRPAmount{},
            STTx{ttVAULT_DEPOSIT, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);

        doInvariantCheck(
            {"updated shares must not exceed maximum"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                adjust(
                    ac.view(),
                    keylet,
                    args(A2.id(), 10, [](Adjustments&) {}),
                    ac.journal);

                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*sleVault)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                (*sleShares)[sfOutstandingAmount] = maxMPTokenAmount + 1;
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_DEPOSIT, [](STObject&) {}},
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);
    }

    void
    testVaultCreate()
    {
        using namespace test::jtx;

        testcase << "Vault create";

        doInvariantCheck(
            {"vault operation updated more than single vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const sequence = ac.view().seq();
                auto const insertVault = [&](Account const A) {
                    auto const vaultKeylet = keylet::vault(A.id(), sequence);
                    auto sleVault = std::make_shared<SLE>(vaultKeylet);
                    auto const vaultPage = ac.view().dirInsert(
                        keylet::ownerDir(A.id()),
                        sleVault->key(),
                        describeOwnerDir(A.id()));
                    sleVault->setFieldU64(sfOwnerNode, *vaultPage);
                    ac.view().insert(sleVault);
                };
                insertVault(A1);
                insertVault(A2);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED});

        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {
                "created vault must be empty",
                "updated zero sized vault must have no assets outstanding",
                "create operation must not have updated a vault",
            },
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                (*sleVault)[sfAssetsTotal] = 9;
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {
                "created vault must be empty",
                "updated zero sized vault must have no assets available",
                "assets available must not be greater than assets outstanding",
                "create operation must not have updated a vault",
            },
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                (*sleVault)[sfAssetsAvailable] = 9;
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {
                "created vault must be enmpty",
                "loss unrealized must not exceed the difference between assets "
                "outstanding and available",
                "vault transaction must not change loss unrealized",
                "create operation must not have updated a vault",
            },
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                (*sleVault)[sfLossUnrealized] = 1;
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {
                "created vault must be empty",
                "create operation must not have updated a vault",
            },
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*sleVault)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                ac.view().update(sleVault);
                (*sleShares)[sfOutstandingAmount] = 9;
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {
                "assets maximum must be positive",
                "create operation must not have updated a vault",
            },
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                (*sleVault)[sfAssetsMaximum] = Number(-1);
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"create operation must not have updated a vault",
             "shares issuer and vault pseudo-account must be the same",
             "shares issuer must be a pseudo-account",
             "shares issuer pseudo-account must point back to the vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*sleVault)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                ac.view().update(sleVault);
                (*sleShares)[sfIssuer] = A1.id();
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"vault created by a wrong transaction type",
             "account root created illegally"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                // The code below will create a valid vault with (almost) all
                // the invariants holding. Except one: it is created by the
                // wrong transaction type.
                auto const sequence = ac.view().seq();
                auto const vaultKeylet = keylet::vault(A1.id(), sequence);
                auto sleVault = std::make_shared<SLE>(vaultKeylet);
                auto const vaultPage = ac.view().dirInsert(
                    keylet::ownerDir(A1.id()),
                    sleVault->key(),
                    describeOwnerDir(A1.id()));
                sleVault->setFieldU64(sfOwnerNode, *vaultPage);

                auto pseudoId =
                    pseudoAccountAddress(ac.view(), vaultKeylet.key);
                // Create pseudo-account.
                auto sleAccount =
                    std::make_shared<SLE>(keylet::account(pseudoId));
                sleAccount->setAccountID(sfAccount, pseudoId);
                sleAccount->setFieldAmount(sfBalance, STAmount{});
                std::uint32_t const seqno =                             //
                    ac.view().rules().enabled(featureSingleAssetVault)  //
                    ? 0                                                 //
                    : sequence;
                sleAccount->setFieldU32(sfSequence, seqno);
                sleAccount->setFieldU32(
                    sfFlags,
                    lsfDisableMaster | lsfDefaultRipple | lsfDepositAuth);
                sleAccount->setFieldH256(sfVaultID, vaultKeylet.key);
                ac.view().insert(sleAccount);

                auto const sharesMptId = makeMptID(sequence, pseudoId);
                auto const sharesKeylet = keylet::mptIssuance(sharesMptId);
                auto sleShares = std::make_shared<SLE>(sharesKeylet);
                auto const sharesPage = ac.view().dirInsert(
                    keylet::ownerDir(pseudoId),
                    sharesKeylet,
                    describeOwnerDir(pseudoId));
                sleShares->setFieldU64(sfOwnerNode, *sharesPage);

                sleShares->at(sfFlags) = 0;
                sleShares->at(sfIssuer) = pseudoId;
                sleShares->at(sfOutstandingAmount) = 0;
                sleShares->at(sfSequence) = sequence;

                sleVault->at(sfAccount) = pseudoId;
                sleVault->at(sfFlags) = 0;
                sleVault->at(sfSequence) = sequence;
                sleVault->at(sfOwner) = A1.id();
                sleVault->at(sfAssetsTotal) = Number(0);
                sleVault->at(sfAssetsAvailable) = Number(0);
                sleVault->at(sfLossUnrealized) = Number(0);
                sleVault->at(sfShareMPTID) = sharesMptId;
                sleVault->at(sfWithdrawalPolicy) =
                    vaultStrategyFirstComeFirstServe;

                ac.view().insert(sleVault);
                ac.view().insert(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject&) {}},
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED});

        doInvariantCheck(
            {"shares issuer and vault pseudo-account must be the same",
             "shares issuer pseudo-account must point back to the vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const sequence = ac.view().seq();
                auto const vaultKeylet = keylet::vault(A1.id(), sequence);
                auto sleVault = std::make_shared<SLE>(vaultKeylet);
                auto const vaultPage = ac.view().dirInsert(
                    keylet::ownerDir(A1.id()),
                    sleVault->key(),
                    describeOwnerDir(A1.id()));
                sleVault->setFieldU64(sfOwnerNode, *vaultPage);

                auto pseudoId =
                    pseudoAccountAddress(ac.view(), vaultKeylet.key);
                // Create pseudo-account.
                auto sleAccount =
                    std::make_shared<SLE>(keylet::account(pseudoId));
                sleAccount->setAccountID(sfAccount, pseudoId);
                sleAccount->setFieldAmount(sfBalance, STAmount{});
                std::uint32_t const seqno =                             //
                    ac.view().rules().enabled(featureSingleAssetVault)  //
                    ? 0                                                 //
                    : sequence;
                sleAccount->setFieldU32(sfSequence, seqno);
                sleAccount->setFieldU32(
                    sfFlags,
                    lsfDisableMaster | lsfDefaultRipple | lsfDepositAuth);
                // sleAccount->setFieldH256(sfVaultID, vaultKeylet.key);
                // Setting wrong vault key
                sleAccount->setFieldH256(sfVaultID, uint256(42));
                ac.view().insert(sleAccount);

                auto const sharesMptId = makeMptID(sequence, pseudoId);
                auto const sharesKeylet = keylet::mptIssuance(sharesMptId);
                auto sleShares = std::make_shared<SLE>(sharesKeylet);
                auto const sharesPage = ac.view().dirInsert(
                    keylet::ownerDir(pseudoId),
                    sharesKeylet,
                    describeOwnerDir(pseudoId));
                sleShares->setFieldU64(sfOwnerNode, *sharesPage);

                sleShares->at(sfFlags) = 0;
                sleShares->at(sfIssuer) = pseudoId;
                sleShares->at(sfOutstandingAmount) = 0;
                sleShares->at(sfSequence) = sequence;

                // sleVault->at(sfAccount) = pseudoId;
                // Setting wrong pseudo account ID
                sleVault->at(sfAccount) = A2.id();
                sleVault->at(sfFlags) = 0;
                sleVault->at(sfSequence) = sequence;
                sleVault->at(sfOwner) = A1.id();
                sleVault->at(sfAssetsTotal) = Number(0);
                sleVault->at(sfAssetsAvailable) = Number(0);
                sleVault->at(sfLossUnrealized) = Number(0);
                sleVault->at(sfShareMPTID) = sharesMptId;
                sleVault->at(sfWithdrawalPolicy) =
                    vaultStrategyFirstComeFirstServe;

                ac.view().insert(sleVault);
                ac.view().insert(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED});

        doInvariantCheck(
            {"shares issuer and vault pseudo-account must be the same",
             "shares issuer must exist"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const sequence = ac.view().seq();
                auto const vaultKeylet = keylet::vault(A1.id(), sequence);
                auto sleVault = std::make_shared<SLE>(vaultKeylet);
                auto const vaultPage = ac.view().dirInsert(
                    keylet::ownerDir(A1.id()),
                    sleVault->key(),
                    describeOwnerDir(A1.id()));
                sleVault->setFieldU64(sfOwnerNode, *vaultPage);

                auto const sharesMptId = makeMptID(sequence, A2.id());
                auto const sharesKeylet = keylet::mptIssuance(sharesMptId);
                auto sleShares = std::make_shared<SLE>(sharesKeylet);
                auto const sharesPage = ac.view().dirInsert(
                    keylet::ownerDir(A2.id()),
                    sharesKeylet,
                    describeOwnerDir(A2.id()));
                sleShares->setFieldU64(sfOwnerNode, *sharesPage);

                sleShares->at(sfFlags) = 0;
                // Setting wrong pseudo account ID
                sleShares->at(sfIssuer) = AccountID(uint160(42));
                sleShares->at(sfOutstandingAmount) = 0;
                sleShares->at(sfSequence) = sequence;

                sleVault->at(sfAccount) = A2.id();
                sleVault->at(sfFlags) = 0;
                sleVault->at(sfSequence) = sequence;
                sleVault->at(sfOwner) = A1.id();
                sleVault->at(sfAssetsTotal) = Number(0);
                sleVault->at(sfAssetsAvailable) = Number(0);
                sleVault->at(sfLossUnrealized) = Number(0);
                sleVault->at(sfShareMPTID) = sharesMptId;
                sleVault->at(sfWithdrawalPolicy) =
                    vaultStrategyFirstComeFirstServe;

                ac.view().insert(sleVault);
                ac.view().insert(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_CREATE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED});
    }

    void
    testVaultSet()
    {
        using namespace test::jtx;

        auto const precloseXrp =
            [&](Account const& A1, Account const& A2, Env& env) -> bool {
            Vault vault{env};
            auto [tx, keylet] =
                vault.create({.owner = A1, .asset = xrpIssue()});
            env(tx);
            env(vault.deposit(
                {.depositor = A1, .id = keylet.key, .amount = XRP(10)}));
            env(vault.deposit(
                {.depositor = A2, .id = keylet.key, .amount = XRP(10)}));
            return true;
        };

        Account const A4{"A4"};
        doInvariantCheck(
            {"set must not change assets outstanding",
             "set must not change assets available",
             "set must not change shares outstanding",
             "set must not change vault balance",
             "assets available must be positive",
             "assets available must not be greater than assets outstanding",
             "assets outstanding must be positive"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                auto slePseudoAccount =
                    ac.view().peek(keylet::account(*(*sleVault)[sfAccount]));
                if (!slePseudoAccount)
                    return false;
                (*slePseudoAccount)[sfBalance] =
                    *(*slePseudoAccount)[sfBalance] - 10;
                ac.view().update(slePseudoAccount);

                // Move 10 drops to A4 to enforce total XRP balance
                auto sleA4 = ac.view().peek(keylet::account(A4.id()));
                if (!sleA4)
                    return false;
                (*sleA4)[sfBalance] = *(*sleA4)[sfBalance] + 10;
                ac.view().update(sleA4);

                return adjust(
                    ac.view(),
                    keylet,
                    args(
                        A2.id(),
                        0,
                        [&](Adjustments& sample) {
                            sample.assetsAvailable =
                                (DROPS_PER_XRP * -100).value();
                            sample.assetsTotal = (DROPS_PER_XRP * -200).value();
                            sample.sharesTotal = -1;
                        }),
                    ac.journal);
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);

        doInvariantCheck(
            {"vault updated by a wrong transaction type"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                ac.view().erase(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttPAYMENT, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"vault updated by a wrong transaction type"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const sequence = ac.view().seq();
                auto const vaultKeylet = keylet::vault(A1.id(), sequence);
                auto sleVault = std::make_shared<SLE>(vaultKeylet);
                auto const vaultPage = ac.view().dirInsert(
                    keylet::ownerDir(A1.id()),
                    sleVault->key(),
                    describeOwnerDir(A1.id()));
                sleVault->setFieldU64(sfOwnerNode, *vaultPage);
                ac.view().insert(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttPAYMENT, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED});

        doInvariantCheck(
            {"vault updated by a wrong transaction type"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttPAYMENT, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"updated vault must have shares"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                (*sleVault)[sfAssetsMaximum] = 200;
                ac.view().update(sleVault);

                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*sleVault)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                ac.view().erase(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject&) {}},
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"violation of vault immutable data"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                sleVault->setFieldIssue(
                    sfAsset, STIssue{sfAsset, MPTIssue(MPTID(42))});
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp);

        doInvariantCheck(
            {"violation of vault immutable data"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                sleVault->setAccountID(sfAccount, A2.id());
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp);

        doInvariantCheck(
            {"violation of vault immutable data"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                (*sleVault)[sfShareMPTID] = MPTID(42);
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp);

        doInvariantCheck(
            {"vault transaction must not change loss unrealized",
             "set must not change assets outstanding"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                return adjust(
                    ac.view(),
                    keylet,
                    args(
                        A2.id(),
                        0,
                        [&](Adjustments& sample) {
                            sample.lossUnrealized = 13;
                            sample.assetsTotal = 20;
                        }),
                    ac.journal);
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);

        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*sleVault)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                // Note, such an "orphaned" update of MPT issuance attached to a
                // vault is invalid; ttVAULT_SET must also update Vault object.
                sleShares->setFieldH256(sfDomainID, uint256(13));
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);

        doInvariantCheck(
            {"set assets outstanding must not exceed assets maximum"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                return adjust(
                    ac.view(),
                    keylet,
                    args(
                        A2.id(),
                        0,
                        [&](Adjustments& sample) { sample.assetsMaximum = 1; }),
                    ac.journal);
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);

        doInvariantCheck(
            {"assets maximum must be positive"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                return adjust(
                    ac.view(),
                    keylet,
                    args(
                        A2.id(),
                        0,
                        [&](Adjustments& sample) {
                            sample.assetsMaximum = -1;
                        }),
                    ac.journal);
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);

        doInvariantCheck(
            {"set must not change shares outstanding",
             "updated zero sized vault must have no assets outstanding",
             "updated zero sized vault must have no assets available"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                ac.view().update(sleVault);
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*sleVault)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                (*sleShares)[sfOutstandingAmount] = 0;
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject& tx) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            precloseXrp,
            TxAccount::A2);
    }

    void
    testVaultDelete()
    {
        using namespace test::jtx;

        doInvariantCheck(
            {"vault deletion succeeded without deleting a vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                ac.view().update(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_DELETE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"vault deleted by a wrong transaction type"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                ac.view().erase(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_SET, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"vault operation updated more than single vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                {
                    auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                    auto sleVault = ac.view().peek(keylet);
                    if (!sleVault)
                        return false;
                    ac.view().erase(sleVault);
                }
                {
                    auto const keylet = keylet::vault(A2.id(), ac.view().seq());
                    auto sleVault = ac.view().peek(keylet);
                    if (!sleVault)
                        return false;
                    ac.view().erase(sleVault);
                }
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_DELETE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                {
                    auto [tx, _] =
                        vault.create({.owner = A1, .asset = xrpIssue()});
                    env(tx);
                }
                {
                    auto [tx, _] =
                        vault.create({.owner = A2, .asset = xrpIssue()});
                    env(tx);
                }
                return true;
            });

        doInvariantCheck(
            {"deleted vault must also delete shares"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                ac.view().erase(sleVault);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_DELETE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });

        doInvariantCheck(
            {"deleted vault must have no shares outstanding",
             "deleted vault must have no assets outstanding",
             "deleted vault must have no assets available"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(A1.id(), ac.view().seq());
                auto sleVault = ac.view().peek(keylet);
                if (!sleVault)
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*sleVault)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                ac.view().erase(sleVault);
                ac.view().erase(sleShares);
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_DELETE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                env(vault.deposit(
                    {.depositor = A1, .id = keylet.key, .amount = XRP(10)}));
                return true;
            });

        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& A1, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            STTx{ttVAULT_DELETE, [](STObject&) {}},
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& A1, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, _] = vault.create({.owner = A1, .asset = xrpIssue()});
                env(tx);
                return true;
            });
    }

     *
     */
public:
    void
    run() override
    {
        // testVault();
        testVaultDeposit();
    }
};

BEAST_DEFINE_TESTSUITE(VaultInvariants, app, xrpl);

}  // namespace test
}  // namespace xrpl
