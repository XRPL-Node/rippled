#include <test/jtx.h>
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

    using CreateTransaction = std::function<STTx(
        test::jtx::Account const& a,
        test::jtx::Account const& b,
        test::jtx::Env& env)>;

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

    enum class AssetType { XRP, MPT, IOU };

    std::vector<AssetType> assetTypes = {
        AssetType::XRP,
        AssetType::MPT,
        AssetType::IOU,
    };

    inline std::string
    to_string(AssetType type)
    {
        switch (type)
        {
            case AssetType::XRP:
                return "XRP";
            case AssetType::MPT:
                return "MPT";
            case AssetType::IOU:
                return "IOU";
        }
        return "Unknown";
    }

    auto static constexpr args =
        [](AccountID id, int adjustment, auto fn) -> Adjustments {
        Adjustments sample = {
            .assetsTotal = adjustment,
            .assetsAvailable = adjustment,
            .lossUnrealized = 0,
            .assetsMaximum = 0,
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
        CreateTransaction ctx =
            [](test::jtx::Account const& A1,
               test::jtx::Account const& A2,
               test::jtx::Env& env) {
                return STTx{ttACCOUNT_SET, [](STObject&) {}};
            },
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

        auto tx = ctx(A1, A2, env);
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
            BEAST_EXPECT(terExpect == terActual);
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
        auto vaultSle = ac.view().peek(keylet);
        if (!BEAST_EXPECT(vaultSle))
            return false;

        auto const mptIssuanceID = (*vaultSle)[sfShareMPTID];
        auto sleShares = ac.view().peek(keylet::mptIssuance(mptIssuanceID));
        if (!BEAST_EXPECT(sleShares))
            return false;

        // These two fields are adjusted in absolute terms
        if (args.lossUnrealized)
            (*vaultSle)[sfLossUnrealized] = *args.lossUnrealized;
        if (args.assetsMaximum)
            (*vaultSle)[sfAssetsMaximum] = *args.assetsMaximum;

        // Remaining fields are adjusted in terms of difference
        if (args.assetsTotal)
            (*vaultSle)[sfAssetsTotal] =
                *(*vaultSle)[sfAssetsTotal] + *args.assetsTotal;
        if (args.assetsAvailable)
            (*vaultSle)[sfAssetsAvailable] =
                *(*vaultSle)[sfAssetsAvailable] + *args.assetsAvailable;
        ac.view().update(vaultSle);

        if (args.sharesTotal)
        {
            (*sleShares)[sfOutstandingAmount] =
                *(*sleShares)[sfOutstandingAmount] + *args.sharesTotal;
            ac.view().update(sleShares);
        }

        auto const assets = *(*vaultSle)[sfAsset];
        auto const pseudoId = *(*vaultSle)[sfAccount];

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
            auto const& pair = *args.accountAssets;
            std::optional<AccountID> sender = std::nullopt;
            AccountID receiver = pair.account;
            STAmount amount = assets.native()
                ? XRPAmount(pair.amount)
                : STAmount{assets, std::abs(pair.amount)};

            if (!assets.native())
            {
                sender = pair.amount > 0 ? assets.getIssuer() : pair.account;
                receiver = pair.amount > 0 ? pair.account : assets.getIssuer();
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

    std::optional<Keylet>
    vaultKeylet(ApplyContext& ac, test::jtx::Account const& acc)
    {
        auto const accSle = ac.view().peek(keylet::account(acc.id()));
        if (!BEAST_EXPECT(accSle))
            return std::nullopt;

        auto const keylet =
            keylet::vault(acc.id(), accSle->getFieldU32(sfSequence) - 2);

        return keylet;
    }

    Preclose
    createEmptyVault()
    {
        using namespace test::jtx;

        return [](Account const& acc, Account const&, Env& env) {
            Vault vault{env};
            auto [tx, _] = vault.create({.owner = acc, .asset = xrpIssue()});
            env(tx, ter(tesSUCCESS));
            return true;
        };
    };

    Preclose
    createVault(AssetType assetType)
    {
        using namespace test::jtx;

        return [&](Account const& alice, Account const& bob, Env& env) -> bool {
            Account issuer{"issuer"};
            env.fund(XRP(1000), issuer);
            env.close();
            PrettyAsset const asset = [&]() {
                switch (assetType)
                {
                    case AssetType::IOU: {
                        PrettyAsset const iouAsset = issuer["IOU"];
                        env(trust(alice, iouAsset(100'000'000)));
                        env(trust(bob, iouAsset(100'000'000)));
                        env(pay(issuer, alice, iouAsset(100'000'000)));
                        env(pay(issuer, bob, iouAsset(100'000'000)));
                        env.close();
                        return iouAsset;
                    }

                    case AssetType::MPT: {
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

                    case AssetType::XRP:
                    default:
                        return PrettyAsset{xrpIssue()};
                }
            }();

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

            auto const amount =              //
                assetType == AssetType::IOU  //
                ? asset(Number{1'234'567, -4})
                : asset(Number{1'234'567});

            depositVaultFunds(alice, amount);
            depositVaultFunds(bob, amount);

            return true;
        };
    }

    void
    testVaultGeneralChecks()
    {
        using namespace jtx;

        std::string const prefix = "Vault general checks";

        testcase(
            prefix + "vault operation succeeded without modifying a vault");
        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_WITHDRAW, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CLAWBACK, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        doInvariantCheck(
            {"vault operation succeeded without updating shares",
             "assets available must not be greater than assets outstanding"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(alice.id(), 0, [&](Adjustments& sample) {
                        sample.assetsTotal = 9;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_WITHDRAW, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));
    }

    void
    testVaultCreate()
    {
        using namespace test::jtx;

        std::string const prefix = "VaultCreate: ";

        testcase(prefix + "vault operation updated more than single vault");
        doInvariantCheck(
            {"vault operation updated more than single vault"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const insertVault = [&](AccountID const& accId) {
                    auto const sequence = ac.view().seq();
                    auto const vaultKeylet = keylet::vault(accId, sequence);
                    auto vaultSle = std::make_shared<SLE>(vaultKeylet);
                    auto const vaultPage = ac.view().dirInsert(
                        keylet::ownerDir(accId),
                        vaultSle->key(),
                        describeOwnerDir(accId));
                    vaultSle->setFieldU64(sfOwnerNode, *vaultPage);
                    ac.view().insert(vaultSle);
                };
                insertVault(alice.id());
                insertVault(bob.id());
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{
                    ttVAULT_CREATE,
                    [](STObject&) {},
                };
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED});

        testcase(
            prefix + "vault operation succeeded without modifying a vault");
        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        testcase(
            prefix +
            "updated zero sized vault must have no assets outstanding");
        doInvariantCheck(
            {
                "created vault must be empty",
                "updated zero sized vault must have no assets outstanding",
                "create operation must not have updated a vault",
            },
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(alice.id(), ac.view().seq());

                return adjust(
                    ac, keylet, args(alice.id(), 0, [&](Adjustments& sample) {
                        sample.assetsTotal = 9;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        testcase(
            prefix + "updated zero sized vault must have no assets available");
        doInvariantCheck(
            {
                "created vault must be empty",
                "updated zero sized vault must have no assets available",
                "assets available must not be greater than assets outstanding",
                "create operation must not have updated a vault",
            },
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(alice.id(), ac.view().seq());

                return adjust(
                    ac, keylet, args(alice.id(), 0, [&](Adjustments& sample) {
                        sample.assetsAvailable = 9;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        testcase(prefix + "vault transaction must not change loss unrealized");
        doInvariantCheck(
            {
                "created vault must be empty",
                "loss unrealized must not exceed the difference between assets "
                "outstanding and available",
                "vault transaction must not change loss unrealized",
                "create operation must not have updated a vault",
            },
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(alice.id(), ac.view().seq());

                return adjust(
                    ac, keylet, args(alice.id(), 0, [&](Adjustments& sample) {
                        sample.lossUnrealized = 9;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        testcase(prefix + "create operation must not have updated a vault");
        doInvariantCheck(
            {
                "created vault must be empty",
                "create operation must not have updated a vault",
            },
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(alice.id(), ac.view().seq());

                auto vaultSle = ac.view().peek(keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*vaultSle)[sfShareMPTID]));
                if (!BEAST_EXPECT(sleShares))
                    return false;

                ac.view().update(vaultSle);
                (*sleShares)[sfOutstandingAmount] = 9;
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        testcase(prefix + "assets maximum must be positive");
        doInvariantCheck(
            {
                "assets maximum must be positive",
                "create operation must not have updated a vault",
            },
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(alice.id(), ac.view().seq());

                auto vaultSle = ac.view().peek(keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;

                (*vaultSle)[sfAssetsMaximum] = Number(-1);
                ac.view().update(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        testcase(prefix + "valid vault pseudo-account");
        doInvariantCheck(
            {"create operation must not have updated a vault",
             "shares issuer and vault pseudo-account must be the same",
             "shares issuer must be a pseudo-account",
             "shares issuer pseudo-account must point back to the vault"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = keylet::vault(alice.id(), ac.view().seq());

                auto vaultSle = ac.view().peek(keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;

                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*vaultSle)[sfShareMPTID]));
                if (!BEAST_EXPECT(sleShares))
                    return false;

                ac.view().update(vaultSle);
                (*sleShares)[sfIssuer] = alice.id();
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createEmptyVault());

        testcase(prefix + "account root created illegally");
        doInvariantCheck(
            {"vault created by a wrong transaction type",
             "account root created illegally"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                // The code below will create a valid vault with (almost) all
                // the invariants holding. Except one: it is created by the
                // wrong transaction type.
                auto const sequence = ac.view().seq();
                auto const vaultKeylet = keylet::vault(alice.id(), sequence);
                auto vaultSle = std::make_shared<SLE>(vaultKeylet);
                auto const vaultPage = ac.view().dirInsert(
                    keylet::ownerDir(alice.id()),
                    vaultSle->key(),
                    describeOwnerDir(alice.id()));
                vaultSle->setFieldU64(sfOwnerNode, *vaultPage);

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

                vaultSle->at(sfAccount) = pseudoId;
                vaultSle->at(sfFlags) = 0;
                vaultSle->at(sfSequence) = sequence;
                vaultSle->at(sfOwner) = alice.id();
                vaultSle->at(sfAssetsTotal) = Number(0);
                vaultSle->at(sfAssetsAvailable) = Number(0);
                vaultSle->at(sfLossUnrealized) = Number(0);
                vaultSle->at(sfShareMPTID) = sharesMptId;
                vaultSle->at(sfWithdrawalPolicy) =
                    vaultStrategyFirstComeFirstServe;

                ac.view().insert(vaultSle);
                ac.view().insert(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED});

        testcase(prefix + "pseudo-account connected to vault");
        doInvariantCheck(
            {"shares issuer and vault pseudo-account must be the same",
             "shares issuer pseudo-account must point back to the vault"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const sequence = ac.view().seq();
                auto const vaultKeylet = keylet::vault(alice.id(), sequence);
                auto vaultSle = std::make_shared<SLE>(vaultKeylet);
                auto const vaultPage = ac.view().dirInsert(
                    keylet::ownerDir(alice.id()),
                    vaultSle->key(),
                    describeOwnerDir(alice.id()));
                vaultSle->setFieldU64(sfOwnerNode, *vaultPage);

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

                // vaultSle->at(sfAccount) = pseudoId;
                // Setting wrong pseudo account ID
                vaultSle->at(sfAccount) = bob.id();
                vaultSle->at(sfFlags) = 0;
                vaultSle->at(sfSequence) = sequence;
                vaultSle->at(sfOwner) = alice.id();
                vaultSle->at(sfAssetsTotal) = Number(0);
                vaultSle->at(sfAssetsAvailable) = Number(0);
                vaultSle->at(sfLossUnrealized) = Number(0);
                vaultSle->at(sfShareMPTID) = sharesMptId;
                vaultSle->at(sfWithdrawalPolicy) =
                    vaultStrategyFirstComeFirstServe;

                ac.view().insert(vaultSle);
                ac.view().insert(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED});

        testcase(prefix + "share issuer mus exist");
        doInvariantCheck(
            {"shares issuer and vault pseudo-account must be the same",
             "shares issuer must exist"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const sequence = ac.view().seq();
                auto const vaultKeylet = keylet::vault(alice.id(), sequence);
                auto vaultSle = std::make_shared<SLE>(vaultKeylet);
                auto const vaultPage = ac.view().dirInsert(
                    keylet::ownerDir(alice.id()),
                    vaultSle->key(),
                    describeOwnerDir(alice.id()));
                vaultSle->setFieldU64(sfOwnerNode, *vaultPage);

                auto const sharesMptId = makeMptID(sequence, bob.id());
                auto const sharesKeylet = keylet::mptIssuance(sharesMptId);
                auto sleShares = std::make_shared<SLE>(sharesKeylet);
                auto const sharesPage = ac.view().dirInsert(
                    keylet::ownerDir(bob.id()),
                    sharesKeylet,
                    describeOwnerDir(bob.id()));
                sleShares->setFieldU64(sfOwnerNode, *sharesPage);

                sleShares->at(sfFlags) = 0;
                // Setting wrong pseudo account ID
                sleShares->at(sfIssuer) = AccountID(uint160(42));
                sleShares->at(sfOutstandingAmount) = 0;
                sleShares->at(sfSequence) = sequence;

                vaultSle->at(sfAccount) = bob.id();
                vaultSle->at(sfFlags) = 0;
                vaultSle->at(sfSequence) = sequence;
                vaultSle->at(sfOwner) = alice.id();
                vaultSle->at(sfAssetsTotal) = Number(0);
                vaultSle->at(sfAssetsAvailable) = Number(0);
                vaultSle->at(sfLossUnrealized) = Number(0);
                vaultSle->at(sfShareMPTID) = sharesMptId;
                vaultSle->at(sfWithdrawalPolicy) =
                    vaultStrategyFirstComeFirstServe;

                ac.view().insert(vaultSle);
                ac.view().insert(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_CREATE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED});
    }

    void
    testVaultSet()
    {
        using namespace test::jtx;

        std::string prefix = "VaultSet: ";

        testcase(prefix + "must not invalidate the vault");
        doInvariantCheck(
            {"set must not change assets outstanding",
             "set must not change assets available",
             "set must not change shares outstanding",
             "set must not change vault balance",
             "assets available must be positive",
             "assets available must not be greater than assets outstanding",
             "assets outstanding must be positive"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 0, [&](Adjustments& sample) {
                        sample.assetsAvailable = (DROPS_PER_XRP * -100).value();
                        sample.assetsTotal = (DROPS_PER_XRP * -200).value();
                        sample.sharesTotal = -1;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP),
            TxAccount::A2);

        testcase(prefix + "vault updated by a wrong transaction type");
        doInvariantCheck(
            {"vault updated by a wrong transaction type"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;

                ac.view().erase(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttPAYMENT, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "vault updated by a wrong transaction type 2");
        doInvariantCheck(
            {"vault updated by a wrong transaction type"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const sequence = ac.view().seq();
                auto const vaultKeylet = keylet::vault(alice.id(), sequence);
                auto vaultSle = std::make_shared<SLE>(vaultKeylet);
                auto const vaultPage = ac.view().dirInsert(
                    keylet::ownerDir(alice.id()),
                    vaultSle->key(),
                    describeOwnerDir(alice.id()));
                vaultSle->setFieldU64(sfOwnerNode, *vaultPage);
                ac.view().insert(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttPAYMENT, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED});

        testcase(prefix + "vault updated by a wrong transaction type 3");
        doInvariantCheck(
            {"vault updated by a wrong transaction type"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                ac.view().update(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttPAYMENT, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "updated vault must have shares");
        doInvariantCheck(
            {"updated vault must have shares"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                (*vaultSle)[sfAssetsMaximum] = 200;
                ac.view().update(vaultSle);

                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*vaultSle)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                ac.view().erase(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "violation of vault immutable data");
        doInvariantCheck(
            {"violation of vault immutable data"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                vaultSle->setFieldIssue(
                    sfAsset, STIssue{sfAsset, MPTIssue(MPTID(42))});
                ac.view().update(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "violation of vault immutable data 2");
        doInvariantCheck(
            {"violation of vault immutable data"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                vaultSle->setAccountID(sfAccount, A2.id());
                ac.view().update(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "violation of vault immutable data 3");
        doInvariantCheck(
            {"violation of vault immutable data"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                (*vaultSle)[sfShareMPTID] = MPTID(42);
                ac.view().update(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "must not change vault balances");
        doInvariantCheck(
            {"vault transaction must not change loss unrealized",
             "set must not change assets outstanding"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(A2.id(), 0, [&](Adjustments& sample) {
                        sample.lossUnrealized = 13;
                        sample.assetsTotal = 20;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP),
            TxAccount::A2);

        testcase(prefix + "must modify the vault");
        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*vaultSle)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                // Note, such an "orphaned" update of MPT issuance attached to a
                // vault is invalid; ttVAULT_SET must also update Vault object.
                sleShares->setFieldH256(sfDomainID, uint256(13));
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP),
            TxAccount::A2);

        testcase(prefix + "must not invalidated assets maximum");
        doInvariantCheck(
            {"set assets outstanding must not exceed assets maximum"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(A2.id(), 0, [&](Adjustments& sample) {
                        sample.assetsMaximum = 1;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP),
            TxAccount::A2);

        testcase(prefix + "assets maximum must be positive");
        doInvariantCheck(
            {"assets maximum must be positive"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(A2.id(), 0, [&](Adjustments& sample) {
                        sample.assetsMaximum = -1;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP),
            TxAccount::A2);

        testcase(prefix + "must not change shares");
        doInvariantCheck(
            {"set must not change shares outstanding",
             "updated zero sized vault must have no assets outstanding",
             "updated zero sized vault must have no assets available"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                ac.view().update(vaultSle);
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*vaultSle)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                (*sleShares)[sfOutstandingAmount] = 0;
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP),
            TxAccount::A2);
    }
    void
    testVaultDelete()
    {
        using namespace test::jtx;

        std::string const prefix = "VaultDelete: ";

        testcase(prefix + "vault deletion succeeded without deleting a vault");
        doInvariantCheck(
            {"vault deletion succeeded without deleting a vault"},
            [&](Account const& alice, Account const&, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto vaultSle = ac.view().peek(*keylet);
                if (!vaultSle)
                    return false;
                ac.view().update(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DELETE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "vault deleted by a wrong transaction type");
        doInvariantCheck(
            {"vault deleted by a wrong transaction type"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto vaultSle = ac.view().peek(*keylet);
                if (!vaultSle)
                    return false;
                ac.view().erase(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_SET, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "vault operation updated more than single vault");
        doInvariantCheck(
            {"vault operation updated more than single vault"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const deleteVault = [&](AccountID const& accId) {
                    // We're not using vaultKeylet here as the accounts in this
                    // testcase have a sequence number of 1
                    auto const keylet = keylet::vault(accId, ac.view().seq());

                    auto vaultSle = ac.view().peek(keylet);
                    if (!BEAST_EXPECT(vaultSle))
                        return false;

                    ac.view().erase(vaultSle);

                    return true;
                };

                BEAST_EXPECT(deleteVault(alice.id()));
                BEAST_EXPECT(deleteVault(bob.id()));
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DELETE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            [&](Account const& alice, Account const& bob, Env& env) {
                Vault vault{env};
                {
                    auto [tx, _] =
                        vault.create({.owner = alice, .asset = xrpIssue()});
                    env(tx);
                }
                {
                    auto [tx, _] =
                        vault.create({.owner = bob, .asset = xrpIssue()});
                    env(tx);
                }
                return true;
            });

        testcase(prefix + "deleted vault must also delete shares");
        doInvariantCheck(
            {"deleted vault must also delete shares"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                ac.view().erase(vaultSle);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DELETE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));

        testcase(prefix + "deleted vault must be empty");
        doInvariantCheck(
            {"deleted vault must have no shares outstanding",
             "deleted vault must have no assets outstanding",
             "deleted vault must have no assets available"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;
                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*vaultSle)[sfShareMPTID]));
                if (!sleShares)
                    return false;
                ac.view().erase(vaultSle);
                ac.view().erase(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DELETE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED},
            [&](Account const& alice, Account const& A2, Env& env) {
                Vault vault{env};
                auto [tx, keylet] =
                    vault.create({.owner = alice, .asset = xrpIssue()});
                env(tx);
                env(vault.deposit(
                    {.depositor = alice, .id = keylet.key, .amount = XRP(10)}));
                return true;
            });

        testcase(
            prefix + "vault operation succeeded without modifying a vault");
        doInvariantCheck(
            {"vault operation succeeded without modifying a vault"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DELETE, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(AssetType::XRP));
    }

    void
    testVaultDeposit(AssetType assetType)
    {
        using namespace test::jtx;

        auto const prefix = "VaultDeposit(" + to_string(assetType) + "): ";

        // no benefit to run this test with multiple asset types
        if (assetType == AssetType::XRP)
        {
            testcase(
                prefix + "vault operation succeeded without modifying a vault");
            doInvariantCheck(
                {"vault operation succeeded without modifying a vault"},
                [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                    return true;
                },
                XRPAmount{},
                [](Account const& A1, Account const& A2, Env& env) {
                    return STTx{ttVAULT_DEPOSIT, [](STObject&) {}};
                },
                {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
                createEmptyVault());
        }

        testcase(prefix + "deposit must change vault balance");
        doInvariantCheck(
            {"deposit must change vault balance"},
            [&](Account const& alice, Account const&, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;
                return adjust(
                    ac, *keylet, args(alice.id(), 0, [](Adjustments& sample) {
                        sample.vaultAssets.reset();
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType));

        testcase(prefix + "deposit must change depositor balance");
        doInvariantCheck(
            {"deposit must change depositor balance"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(alice.id(), 10, [&](Adjustments& sample) {
                        // cancel out the account asset change
                        sample.accountAssets->amount = -10;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject& tx) {
                                tx[sfAmount] = XRPAmount(10);
                            }};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "deposit and assets outstanding must add up");
        doInvariantCheck(
            {"deposit and assets outstanding must add up"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 10, [&](Adjustments& sample) {
                        sample.assetsTotal = 11;
                    }));
            },
            XRPAmount{},
            [](Account const&, Account const&, Env&) {
                return STTx{ttVAULT_DEPOSIT, [&](STObject& tx) {
                                // prevent triggering
                                // Invariant failed: deposit must not change
                                // vault balance by more than deposited
                                // amount

                                tx[sfAmount] = XRPAmount(10);
                            }};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(
            prefix +
            "deposit assets outstanding must not exceed assets maximum");
        doInvariantCheck(
            {"deposit assets outstanding must not exceed assets maximum"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 200, [&](Adjustments& sample) {
                        sample.assetsMaximum = 1;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject& tx) {
                                tx.setFieldAmount(sfAmount, XRPAmount(200));
                            }};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "deposit must change depositor shares");
        doInvariantCheck(
            {"deposit must change depositor shares"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 10, [&](Adjustments& sample) {
                        sample.accountShares.reset();
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject& tx) {
                                tx[sfAmount] = XRPAmount(10);
                            }};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "deposit must change vault shares");
        doInvariantCheck(
            {"deposit must change vault shares"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 10, [](Adjustments& sample) {
                        sample.sharesTotal = 0;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject& tx) {
                                tx[sfAmount] = XRPAmount(10);
                            }};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "deposit must change vault and depositor balances");
        doInvariantCheck(
            {"deposit must increase vault balance",
             "deposit must change depositor balance"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(alice.id(), -10, [&](Adjustments&) {}));
            },
            XRPAmount{},
            [&](Account const&, Account const&, Env&) {
                return STTx{ttVAULT_DEPOSIT, [&](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "deposit must not change loss unrealized");
        doInvariantCheck(
            {"loss unrealized must not exceed the difference "
             "between assets outstanding and available",
             "vault transaction must not change loss unrealized"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 100, [&](Adjustments& sample) {
                        sample.lossUnrealized = 13;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject& tx) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "deposit must change vault and depositor shares");
        doInvariantCheck(
            {"deposit must increase depositor shares",
             "deposit must change depositor and vault shares by equal "
             "amount",
             "deposit must not change vault balance by more than deposited "
             "amount"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 10, [&](Adjustments& sample) {
                        sample.accountShares->amount = -5;
                        sample.sharesTotal = -10;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject& tx) {
                                tx[sfAmount] = XRPAmount(5);
                            }};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "deposit and vault balances must add up");
        doInvariantCheck(
            {
                "deposit must increase vault balance",
                "deposit must decrease depositor balance",
                "deposit must change vault and depositor balance by equal "
                "amount",
                "deposit and assets outstanding must add up",
                "deposit and assets available must add up",
            },
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto const vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;

                auto const asset = *vaultSle->at(sfAsset);

                if (!BEAST_EXPECT(adjustBalance(
                        ac,
                        asset.getIssuer(),
                        alice,
                        STAmount{asset, Number{10, 0}})))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 10, [&](Adjustments& sample) {
                        sample.vaultAssets = -20;
                        sample.accountAssets->amount = 10;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject& tx) {
                                tx[sfAmount] = XRPAmount(10);
                            }};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "vault balances must add up");
        doInvariantCheck(
            {"deposit and assets outstanding must add up",
             "deposit and assets available must add up"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                return adjust(
                    ac, *keylet, args(bob.id(), 10, [&](Adjustments& sample) {
                        sample.assetsTotal = 7;
                        sample.assetsAvailable = 7;
                    }));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject& tx) {
                                tx[sfAmount] = XRPAmount(10);
                            }};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "updated shares must not exceed maximum 1");
        doInvariantCheck(
            {"updated shares must not exceed maximum"},
            [&](Account const& alice, Account const& bob, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;

                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*vaultSle)[sfShareMPTID]));
                if (!BEAST_EXPECT(sleShares))
                    return false;

                (*sleShares)[sfMaximumAmount] = 10;
                ac.view().update(sleShares);

                return adjust(
                    ac, *keylet, args(bob.id(), 10, [](Adjustments&) {}));
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);

        testcase(prefix + "updated shares must not exceed maximum 2");
        doInvariantCheck(
            {"updated shares must not exceed maximum"},
            [&](Account const& alice, Account const& A2, ApplyContext& ac) {
                auto const keylet = vaultKeylet(ac, alice);
                if (!BEAST_EXPECT(keylet))
                    return false;

                auto vaultSle = ac.view().peek(*keylet);
                if (!BEAST_EXPECT(vaultSle))
                    return false;

                auto sleShares = ac.view().peek(
                    keylet::mptIssuance((*vaultSle)[sfShareMPTID]));
                if (!BEAST_EXPECT(sleShares))
                    return false;

                (*sleShares)[sfOutstandingAmount] = maxMPTokenAmount + 1;
                ac.view().update(sleShares);
                return true;
            },
            XRPAmount{},
            [](Account const& A1, Account const& A2, Env& env) {
                return STTx{ttVAULT_DEPOSIT, [](STObject&) {}};
            },
            {tecINVARIANT_FAILED, tefINVARIANT_FAILED},
            createVault(assetType),
            TxAccount::A2);
    }
    // void
    // testVault()
    // {

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
    //         [](Account const& A1, Account const& A2, Env& env) {
    //             return STTx{ttVAULT_WITHDRAW, [](STObject&) {}};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP));

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
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A3{"A3"};
    //             return STTx{
    //                 ttVAULT_WITHDRAW,
    //                 [&](STObject& tx) {
    //                     tx[sfFee] = XRPAmount(100);
    //                     tx[sfAccount] = A3.id();
    //                     // This commented out line causes the invariant
    //                     // violation. tx[sfDestination] = A4.id();
    //                 }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP));

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
    //         [](Account const& A1, Account const& A2, Env& env) {
    //             return STTx{ttVAULT_WITHDRAW, [](STObject&) {}};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP),
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
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A3{"A3"};
    //             return STTx{
    //                 ttVAULT_WITHDRAW,
    //                 [&](STObject& tx) { tx.setAccountID(sfDestination,
    //                 A3.id());
    //                 }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP),
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
    //         [](Account const& A1, Account const& A2, Env& env) {
    //             return STTx{ttVAULT_WITHDRAW, [](STObject&) {}};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP),
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
    //         [](Account const& A1, Account const& A2, Env& env) {
    //             return STTx{ttVAULT_WITHDRAW, [](STObject&) {}};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP),
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
    //         [](Account const& A1, Account const& A2, Env& env) {
    //             return STTx{ttVAULT_WITHDRAW, [](STObject&) {}};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP),
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
    //         [](Account const& A1, Account const& A2, Env& env) {
    //             return STTx{ttVAULT_WITHDRAW, [](STObject&) {}};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP),
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
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A3{"A3"};
    //             return STTx{
    //                 ttVAULT_WITHDRAW,
    //                 [&](STObject& tx) {
    //                     tx[sfAmount] = XRPAmount(10);
    //                     tx[sfDelegate] = A3.id();
    //                     tx[sfFee] = XRPAmount(2000);
    //                 }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP),
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
    //         auto [tx, keylet] = vault.create({.owner = alice, .asset =
    //         asset}); env(tx); env(vault.deposit(
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
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A3{"A3"};
    //             return STTx{
    //                 ttVAULT_WITHDRAW,
    //                 [&](STObject& tx) { tx[sfAccount] = A3.id(); }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt,
    //         TxAccount::A2);

    //     testcase << "Vault clawback";
    //     doInvariantCheck(
    //         {"clawback must change vault balance"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), -1, [&](Adjustments& sample) {
    //                     sample.vaultAssets.reset();
    //                 }));
    //         },
    //         XRPAmount{},
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A3{"A3"};
    //             return STTx{
    //                 ttVAULT_CLAWBACK,
    //                 [&](STObject& tx) { tx[sfAccount] = A3.id(); }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);

    //     // Not the same as below check: attempt to clawback XRP
    //     doInvariantCheck(
    //         {"clawback may only be performed by the asset issuer"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq());
    //             return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), 0, [&](Adjustments& sample) {}));
    //         },
    //         XRPAmount{},
    //         [](Account const& A1, Account const& A2, Env& env) {
    //             return STTx{ttVAULT_CLAWBACK, [](STObject&) {}};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         createVault(AssetType::XRP));

    //     // Not the same as above check: attempt to clawback MPT by bad
    //     // account
    //     doInvariantCheck(
    //         {"clawback may only be performed by the asset issuer"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A2.id(), 0, [&](Adjustments& sample) {}));
    //         },
    //         XRPAmount{},
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A4{"A4"};
    //             return STTx{
    //                 ttVAULT_CLAWBACK,
    //                 [&](STObject& tx) { tx[sfAccount] = A4.id(); }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);

    //     doInvariantCheck(
    //         {"clawback must decrease vault balance",
    //          "clawback must decrease holder shares",
    //          "clawback must change vault shares"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A4.id(), 10, [&](Adjustments& sample) {
    //                     sample.sharesTotal = 0;
    //                 }));
    //         },
    //         XRPAmount{},
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A3{"A3"};
    //             Account A4{"A4"};
    //             return STTx{
    //                 ttVAULT_CLAWBACK,
    //                 [&](STObject& tx) {
    //                     tx[sfAccount] = A3.id();
    //                     tx[sfHolder] = A4.id();
    //                 }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);

    //     doInvariantCheck(
    //         {"clawback must change holder shares"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A4.id(), -10, [&](Adjustments& sample) {
    //                     sample.accountShares.reset();
    //                 }));
    //         },
    //         XRPAmount{},
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A3{"A3"};
    //             Account A4{"A4"};
    //             return STTx{
    //                 ttVAULT_CLAWBACK,
    //                 [&](STObject& tx) {
    //                     tx[sfAccount] = A3.id();
    //                     tx[sfHolder] = A4.id();
    //                 }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);

    //     doInvariantCheck(
    //         {"clawback must change holder and vault shares by equal amount",
    //          "clawback and assets outstanding must add up",
    //          "clawback and assets available must add up"},
    //         [&](Account const& A1, Account const& A2, ApplyContext& ac) {
    //             auto const keylet = keylet::vault(A1.id(), ac.view().seq() -
    //             2); return adjust(
    //                 ac,
    //                 keylet,
    //                 args(A4.id(), -10, [&](Adjustments& sample) {
    //                     sample.accountShares->amount = -8;
    //                     sample.assetsTotal = -7;
    //                     sample.assetsAvailable = -7;
    //                 }));
    //         },
    //         XRPAmount{},
    //         [&](Account const& A1, Account const& A2, Env& env) {
    //             Account A3{"A3"};
    //             Account A4{"A4"};
    //             return STTx{
    //                 ttVAULT_CLAWBACK,
    //                 [&](STObject& tx) {
    //                     tx[sfAccount] = A3.id();
    //                     tx[sfHolder] = A4.id();
    //                 }};
    //         },
    //         {tecINVARIANT_FAILED, tecINVARIANT_FAILED},
    //         precloseMpt);
    // }

public:
    void
    run() override
    {
        testVaultGeneralChecks();
        testVaultCreate();
        testVaultSet();
        testVaultDelete();
        for (auto const assetType : assetTypes)
            testVaultDeposit(assetType);
    }
};

BEAST_DEFINE_TESTSUITE(VaultInvariants, app, xrpl);

}  // namespace test
}  // namespace xrpl
