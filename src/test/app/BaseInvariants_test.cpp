#include <test/jtx.h>
#include <test/jtx/Env.h>

#include <xrpld/app/tx/detail/ApplyContext.h>

#include <xrpl/beast/unit_test/suite.h>

namespace xrpl {
namespace test {

// A minimal utility class providing necessary artifacts to test invariants
class BaseInvariants_test : public beast::unit_test::suite
{
protected:
    /**
     * Enumeration of asset types. Used for invariant tests that require
     * multiple asset types.
     */
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
            BEAST_EXPECT(terExpect == terActual);
            auto const messages = sink.messages().str();
            BEAST_EXPECT(
                messages.starts_with("Invariant failed:") ||
                messages.starts_with("Transaction caused an exception"));
            // std::cerr << messages << '\n';
            for (auto const& m : expect_logs)
            {
                if (messages.find(m) == std::string::npos)
                {
                    // uncomment if you want to log the invariant failure
                    // std::cerr << "   --> " << m << std::endl;
                    fail();
                }
            }
        }
    }
};
}  // namespace test
}  // namespace xrpl
