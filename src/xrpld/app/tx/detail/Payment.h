#ifndef XRPL_TX_PAYMENT_H_INCLUDED
#define XRPL_TX_PAYMENT_H_INCLUDED

#include <xrpld/app/tx/detail/Transactor.h>

namespace ripple {

class Payment : public Transactor
{
    /* The largest number of paths we allow */
    static std::size_t const MaxPathSize = 6;

    /* The longest path we allow */
    static std::size_t const MaxPathLength = 8;

public:
    static constexpr ConsequencesFactoryType ConsequencesFactory{Custom};

    explicit Payment(ApplyContext& ctx) : Transactor(ctx)
    {
    }

    static TxConsequences
    makeTxConsequences(PreflightContext const& ctx);

    static bool
    checkExtraFeatures(PreflightContext const& ctx);

    static std::uint32_t
    getFlagsMask(PreflightContext const& ctx);

    static NotTEC
    preflight(PreflightContext const& ctx);

    static NotTEC
    checkPermission(ReadView const& view, STTx const& tx);

    static TER
    preclaim(PreclaimContext const& ctx);

    TER
    doApply() override;

    // Helpers for this class and other transactors that make "Payments"
    struct RipplePaymentParams
    {
        ApplyContext& ctx;
        std::optional<STAmount> const& sendMax;
        AccountID const& srcAccountID;
        AccountID const& dstAccountID;
        STAmount const& dstAmount;
        XRPAmount const& sourceBalance;
        XRPAmount const& priorBalance;
        STPathSet const& paths;
        std::optional<STAmount> const& deliverMin;
        std::optional<uint256> const& domainID;
        std::uint32_t flags = 0;
    };

    static TER
    makePayment(RipplePaymentParams& p);
};

}  // namespace ripple

#endif
