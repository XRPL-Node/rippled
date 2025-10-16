//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2025 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_APP_MISC_LENDINGHELPERS_H_INCLUDED
#define RIPPLE_APP_MISC_LENDINGHELPERS_H_INCLUDED

#include <xrpl/ledger/View.h>
#include <xrpl/protocol/st.h>

namespace ripple {

struct PreflightContext;

// Lending protocol has dependencies, so capture them here.
bool
checkLendingProtocolDependencies(PreflightContext const& ctx);

Number
loanPeriodicRate(TenthBips32 interestRate, std::uint32_t paymentInterval);

// This structure is used internally to compute the breakdown of a
// single loan payment
struct PaymentComponents
{
    Number rawInterest;
    Number rawPrincipal;
    Number roundedInterest;
    Number roundedPrincipal;
    // We may not need roundedPayment
    Number roundedPayment;
    bool final = false;
    bool extra = false;
};

// This structure is explained in the XLS-66 spec, section 3.2.4.4 (Failure
// Conditions)
struct LoanPaymentParts
{
    /// principal_paid is the amount of principal that the payment covered.
    Number principalPaid;
    /// interest_paid is the amount of interest that the payment covered.
    Number interestPaid;
    /**
     * value_change is the amount by which the total value of the Loan changed.
     *  If value_change < 0, Loan value decreased.
     *  If value_change > 0, Loan value increased.
     * This is 0 for regular payments.
     */
    Number valueChange;
    /// fee_paid is the amount of fee that the payment covered.
    Number feeToPay;

    LoanPaymentParts&
    operator+=(LoanPaymentParts const& other)
    {
        principalPaid += other.principalPaid;
        interestPaid += other.interestPaid;
        valueChange += other.valueChange;
        feeToPay += other.feeToPay;
        return *this;
    }
};

/// Ensure the periodic payment is always rounded consistently
template <AssetType A>
Number
roundPeriodicPayment(
    A const& asset,
    Number const& periodicPayment,
    std::int32_t scale)
{
    return roundToAsset(asset, periodicPayment, scale, Number::upward);
}

namespace detail {
// These functions should rarely be used directly. More often, the ultimate
// result needs to be roundToAsset'd.

Number
loanPeriodicPayment(
    Number const& principalOutstanding,
    Number const& periodicRate,
    std::uint32_t paymentsRemaining);

Number
loanPeriodicPayment(
    Number const& principalOutstanding,
    TenthBips32 interestRate,
    std::uint32_t paymentInterval,
    std::uint32_t paymentsRemaining);

Number
loanPrincipalFromPeriodicPayment(
    Number const& periodicPayment,
    Number const& periodicRate,
    std::uint32_t paymentsRemaining);

Number
loanLatePaymentInterest(
    Number const& principalOutstanding,
    TenthBips32 lateInterestRate,
    NetClock::time_point parentCloseTime,
    std::uint32_t nextPaymentDueDate);

Number
loanAccruedInterest(
    Number const& principalOutstanding,
    Number const& periodicRate,
    NetClock::time_point parentCloseTime,
    std::uint32_t startDate,
    std::uint32_t prevPaymentDate,
    std::uint32_t paymentInterval);

inline Number
minusFee(Number const& value, TenthBips32 managementFeeRate)
{
    return tenthBipsOfValue(value, tenthBipsPerUnity - managementFeeRate);
}

template <AssetType A>
PaymentComponents
computePaymentComponents(
    A const& asset,
    std::int32_t scale,
    Number const& totalValueOutstanding,
    Number const& principalOutstanding,
    Number const& periodicPayment,
    Number const& periodicRate,
    std::uint32_t paymentRemaining)
{
    /*
     * This function is derived from the XLS-66 spec, section 3.2.4.1.1 (Regular
     * Payment)
     */
    XRPL_ASSERT_PARTS(
        isRounded(asset, totalValueOutstanding, scale) &&
            isRounded(asset, principalOutstanding, scale),
        "ripple::detail::computePaymentComponents",
        "Outstanding values are rounded");
    auto const roundedPeriodicPayment =
        roundPeriodicPayment(asset, periodicPayment, scale);
    if (paymentRemaining == 1 ||
        totalValueOutstanding <= roundedPeriodicPayment)
    {
        // If there's only one payment left, we need to pay off each of the loan
        // parts. It's probably impossible for the subtraction to result in a
        // negative value, but don't leave anything to chance.
        Number interest =
            std::max(Number{}, totalValueOutstanding - principalOutstanding);

        // Pay everything off
        return {
            .rawInterest = interest,
            .rawPrincipal = principalOutstanding,
            .roundedInterest = interest,
            .roundedPrincipal = principalOutstanding,
            .roundedPayment = interest + principalOutstanding,
            .final = true};
    }

    Number const rawValueOutstanding = periodicPayment * paymentRemaining;
    Number const rawPrincipalOutstanding = loanPrincipalFromPeriodicPayment(
        periodicPayment, periodicRate, paymentRemaining);
    Number const rawInterestOutstanding =
        rawValueOutstanding - rawPrincipalOutstanding;

    /*
     * From the spec, once the periodicPayment is computed:
     *
     * The principal and interest portions can be derived as follows:
     *  interest = principalOutstanding * periodicRate
     *  principal = periodicPayment - interest
     */
    Number const rawInterest = rawPrincipalOutstanding * periodicRate;
    Number const rawPrincipal = periodicPayment - rawInterest;
    XRPL_ASSERT_PARTS(
        rawInterest >= 0,
        "ripple::detail::computePaymentComponents",
        "valid raw interest");
    XRPL_ASSERT_PARTS(
        rawPrincipal >= 0 && rawPrincipal <= rawPrincipalOutstanding,
        "ripple::detail::computePaymentComponents",
        "valid raw principal");

    /*
        Critical Calculation: Balancing Principal and Interest Outstanding

        This calculation maintains a delicate balance between keeping
        principal outstanding and interest outstanding as close as possible to
        reference values. However, we cannot perfectly match the reference
       values due to rounding issues.

        Key considerations:
            1. Since the periodic payment is rounded up, we have excess funds
               that can be used to pay down the loan faster than the reference
               calculation.

            2. We must ensure that loan repayment is not too fast, otherwise we
               will end up with negative principal outstanding or negative
       interest outstanding.

            3. We cannot allow the borrower to repay interest ahead of schedule.
               If the borrower makes an overpayment, the interest portion could
       go negative, requiring complex recalculation to refund the borrower by
               reflecting the overpayment in the principal portion of the loan.
    */

    Number const roundedPrincipal = [&]() {
        auto const p = roundToAsset(
            asset,
            // Compute the delta that will get the tracked principalOutstanding
            // amount as close to the true principal amount after the payment as
            // possible.
            principalOutstanding - (rawPrincipalOutstanding - rawPrincipal),
            scale,
            Number::downward);

        // The principal part can only be 0 during intial loan validation. If it
        // is 0, the Loan will not be created, but we don't want an assert
        // aborting the process before we get that far.
        XRPL_ASSERT_PARTS(
            p >= 0,
            "rippled::detail::computePaymentComponents",
            "principal part not negative");
        XRPL_ASSERT_PARTS(
            p <= principalOutstanding,
            "rippled::detail::computePaymentComponents",
            "principal part not larger than outstanding principal");
        XRPL_ASSERT_PARTS(
            p <= roundedPeriodicPayment,
            "rippled::detail::computePaymentComponents",
            "principal part not larger than total payment");

        // Make sure nothing goes negative
        if (p > roundedPeriodicPayment || p > principalOutstanding)
            return std::min(roundedPeriodicPayment, principalOutstanding);
        else if (p < 0)
            return Number{};

        return p;
    }();

    Number const roundedInterest = [&]() {
        // Zero interest means ZERO interest
        if (periodicRate == 0)
            return Number{};

        // Compute the rounded interest outstanding
        auto const interestOutstanding =
            totalValueOutstanding - principalOutstanding;
        // Compute the delta that will simply treat the rest of the rounded
        // fixed payment amount as interest.
        auto const iDiff = roundedPeriodicPayment - roundedPrincipal;

        // Compute the delta that will get the untracked interestOutstanding
        // amount as close as possible to the true interest amount after the
        // payment as possible.
        auto const iSync = interestOutstanding -
            (roundToAsset(asset, rawInterestOutstanding, scale) -
             roundToAsset(asset, rawInterest, scale));
        XRPL_ASSERT_PARTS(
            isRounded(asset, iSync, scale),
            "ripple::detail::computePaymentComponents",
            "iSync is rounded");

        // Use the smaller of the two to ensure we don't overpay interest.
        auto const i = std::min({iSync, iDiff, interestOutstanding});

        // No negative interest!
        if (i < 0)
            return Number{};
        return i;
    }();

    XRPL_ASSERT_PARTS(
        roundedInterest >= 0 && isRounded(asset, roundedInterest, scale),
        "ripple::detail::computePaymentComponents",
        "valid rounded interest");
    XRPL_ASSERT_PARTS(
        roundedPrincipal >= 0 && roundedPrincipal <= principalOutstanding &&
            roundedPrincipal <= roundedPeriodicPayment &&
            isRounded(asset, roundedPrincipal, scale),
        "ripple::detail::computePaymentComponents",
        "valid rounded principal");
    XRPL_ASSERT_PARTS(
        roundedPrincipal + roundedInterest <= roundedPeriodicPayment,
        "ripple::detail::computePaymentComponents",
        "payment parts fit within payment limit");

    return {
        .rawInterest = rawInterest,
        .rawPrincipal = rawPrincipal,
        .roundedInterest = roundedInterest,
        .roundedPrincipal = roundedPrincipal,
        .roundedPayment = roundedPeriodicPayment};
}

struct PaymentComponentsPlus : public PaymentComponents
{
    Number fee{0};
    Number valueChange{0};

    PaymentComponentsPlus(
        PaymentComponents const& p,
        Number f,
        Number v = Number{})
        : PaymentComponents(p), fee(f), valueChange(v)
    {
    }
};

template <class NumberProxy, class UInt32Proxy, class UInt32OptionalProxy>
LoanPaymentParts
doPayment(
    PaymentComponentsPlus const& payment,
    NumberProxy& totalValueOutstandingProxy,
    NumberProxy& principalOutstandingProxy,
    UInt32Proxy& paymentRemainingProxy,
    UInt32Proxy& prevPaymentDateProxy,
    UInt32OptionalProxy& nextDueDateProxy,
    std::uint32_t paymentInterval)
{
    XRPL_ASSERT_PARTS(
        nextDueDateProxy,
        "ripple::detail::doPayment",
        "Next due date proxy set");
    auto const totalValueDelta = payment.roundedPrincipal +
        payment.roundedInterest - payment.valueChange;
    if (!payment.extra)
    {
        if (payment.final)
        {
            XRPL_ASSERT_PARTS(
                principalOutstandingProxy == payment.roundedPrincipal,
                "ripple::detail::doPayment",
                "Full principal payment");
            XRPL_ASSERT_PARTS(
                totalValueOutstandingProxy == totalValueDelta,
                "ripple::detail::doPayment",
                "Full value payment");

            paymentRemainingProxy = 0;

            prevPaymentDateProxy = *nextDueDateProxy;
            // Remove the field. This is the only condition where nextDueDate is
            // allowed to be removed.
            nextDueDateProxy = std::nullopt;
        }
        else
        {
            XRPL_ASSERT_PARTS(
                principalOutstandingProxy > payment.roundedPrincipal,
                "ripple::detail::doPayment",
                "Full principal payment");
            XRPL_ASSERT_PARTS(
                totalValueOutstandingProxy > totalValueDelta,
                "ripple::detail::doPayment",
                "Full value payment");

            paymentRemainingProxy -= 1;

            prevPaymentDateProxy = *nextDueDateProxy;
            // STObject::OptionalField does not define operator+=, so do it the
            // old-fashioned way.
            nextDueDateProxy = *nextDueDateProxy + paymentInterval;
        }
    }

    principalOutstandingProxy -= payment.roundedPrincipal;
    totalValueOutstandingProxy -= totalValueDelta;

    XRPL_ASSERT_PARTS(
        // Use an explicit cast because the template parameter can be
        // ValueProxy<Number> or Number
        static_cast<Number>(principalOutstandingProxy) <=
            static_cast<Number>(totalValueOutstandingProxy),
        "ripple::detail::doPayment",
        "principal does not exceed total");

    return LoanPaymentParts{
        .principalPaid = payment.roundedPrincipal,
        .interestPaid = payment.roundedInterest,
        .valueChange = payment.valueChange,
        .feeToPay = payment.fee};
}

template <
    AssetType A,
    class NumberProxy,
    class UInt32Proxy,
    class UInt32OptionalProxy>
Expected<LoanPaymentParts, TER>
doOverpayment(
    A const& asset,
    ApplyView& view,
    PaymentComponentsPlus const& overpaymentComponents,
    NumberProxy& totalValueOutstandingProxy,
    NumberProxy& principalOutstandingProxy,
    NumberProxy& periodicPaymentProxy,
    TenthBips32 const interestRate,
    std::uint32_t const paymentInterval,
    UInt32Proxy& paymentRemainingProxy,
    UInt32Proxy& prevPaymentDateProxy,
    UInt32OptionalProxy& nextDueDateProxy,
    TenthBips16 managementFeeRate,
    beast::Journal j)
{
    Number const totalInterestOutstandingBefore =
        totalValueOutstandingProxy - principalOutstandingProxy;

    // Compute what the properties would be if the loan was new in its current
    // state. They are not likely to match the original properties. We're
    // interested in the error.
    auto const oldLoanProperties = computeLoanProperties(
        asset,
        principalOutstandingProxy,
        interestRate,
        paymentInterval,
        paymentRemainingProxy,
        managementFeeRate);

    auto const accumulatedError =
        oldLoanProperties.totalValueOutstanding - totalValueOutstandingProxy;

    {
        // Use temp variables to do the payment, so they can be thrown away if
        // they don't work
        Number totalValueOutstanding = totalValueOutstandingProxy;
        Number principalOutstanding = principalOutstandingProxy;
        std::uint32_t paymentRemaining = paymentRemainingProxy;
        std::uint32_t prevPaymentDate = prevPaymentDateProxy;
        std::optional<std::uint32_t> nextDueDate = nextDueDateProxy;

        auto const paymentParts = detail::doPayment(
            overpaymentComponents,
            totalValueOutstanding,
            principalOutstanding,
            paymentRemaining,
            prevPaymentDate,
            nextDueDate,
            paymentInterval);

        auto newLoanProperties = computeLoanProperties(
            asset,
            principalOutstanding,
            interestRate,
            paymentInterval,
            paymentRemaining,
            managementFeeRate);

        newLoanProperties.totalValueOutstanding += accumulatedError;

        if (newLoanProperties.firstPaymentPrincipal <= 0 &&
            principalOutstanding > 0)
        {
            // The overpayment has caused the loan to be in a state
            // where no further principal can be paid.
            JLOG(j.warn())
                << "Loan overpayment would cause loan to be stuck. "
                   "Rejecting overpayment, but normal payments are unaffected.";
            return Unexpected(tesSUCCESS);
        }
        // Check that the other computed values are valid
        if (newLoanProperties.interestOwedToVault < 0 ||
            newLoanProperties.totalValueOutstanding <= 0 ||
            newLoanProperties.periodicPayment <= 0)
        {
            // LCOV_EXCL_START
            JLOG(j.warn()) << "Computed loan properties are invalid. Does "
                              "not compute. TotalValueOutstanding: "
                           << newLoanProperties.totalValueOutstanding
                           << ", PeriodicPayment: "
                           << newLoanProperties.periodicPayment
                           << ", InterestOwedToVault: "
                           << newLoanProperties.interestOwedToVault;
            return Unexpected(tesSUCCESS);
            // LCOV_EXCL_STOP
        }

        totalValueOutstandingProxy =
            newLoanProperties.totalValueOutstanding + accumulatedError;
        principalOutstandingProxy = principalOutstanding;
        periodicPaymentProxy = newLoanProperties.periodicPayment;

        XRPL_ASSERT_PARTS(
            paymentRemainingProxy == paymentRemaining,
            "ripple::detail::doOverpayment",
            "paymentRemaining is unchanged");
        paymentRemainingProxy = paymentRemaining;
        XRPL_ASSERT_PARTS(
            prevPaymentDateProxy == prevPaymentDate,
            "ripple::detail::doOverpayment",
            "prevPaymentDate is unchanged");
        prevPaymentDateProxy = prevPaymentDate;
        XRPL_ASSERT_PARTS(
            nextDueDateProxy == nextDueDate,
            "ripple::detail::doOverpayment",
            "nextDueDate is unchanged");
        nextDueDateProxy = nextDueDate;

        /*
        auto const totalInterestOutstandingAfter =
            totalValueOutstanding - principalOutstanding;
        */

        return paymentParts;
    }
}

/* Handle possible late payments.
 *
 * If this function processed a late payment, the return value will be
 * a LoanPaymentParts object. If the loan is not late, the return will be an
 * Unexpected(tesSUCCESS). Otherwise, it'll be an Unexpected with the error code
 * the caller is expected to return.
 *
 *
 * This function is an implementation of the XLS-66 spec, based on
 * * section 3.2.4.3 (Transaction Pseudo-code), specifically the bit
 *   labeled "the payment is late"
 * * section 3.2.4.1.2 (Late Payment)
 */
template <AssetType A>
Expected<PaymentComponentsPlus, TER>
handleLatePayment(
    A const& asset,
    ApplyView const& view,
    Number const& principalOutstanding,
    std::int32_t nextDueDate,
    PaymentComponentsPlus const& periodic,
    TenthBips32 lateInterestRate,
    std::int32_t loanScale,
    Number const& latePaymentFee,
    STAmount const& amount,
    beast::Journal j)
{
    if (!hasExpired(view, nextDueDate))
        return Unexpected(tesSUCCESS);

    // the payment is late
    // Late payment interest is only the part of the interest that comes from
    // being late, as computed by 3.2.4.1.2.
    auto const latePaymentInterest = loanLatePaymentInterest(
        asset,
        principalOutstanding,
        lateInterestRate,
        view.parentCloseTime(),
        nextDueDate,
        loanScale);
    XRPL_ASSERT(
        latePaymentInterest >= 0,
        "ripple::detail::handleLatePayment : valid late interest");
    PaymentComponentsPlus const late{
        PaymentComponents{
            .rawInterest = periodic.rawInterest + latePaymentInterest,
            .rawPrincipal = periodic.rawPrincipal,
            .roundedInterest = periodic.roundedInterest + latePaymentInterest,
            .roundedPrincipal = periodic.roundedPrincipal,
            .roundedPayment = periodic.roundedPayment},
        // A late payment pays both the normal fee, and the extra fee
        periodic.fee + latePaymentFee,
        // A late payment increases the value of the loan by the difference
        // between periodic and late payment interest
        latePaymentInterest};
    auto const totalDue =
        late.roundedPrincipal + late.roundedInterest + late.fee;
    XRPL_ASSERT_PARTS(
        isRounded(asset, totalDue, loanScale),
        "ripple::detail::handleLatePayment",
        "total due is rounded");

    if (amount < totalDue)
    {
        JLOG(j.warn()) << "Late loan payment amount is insufficient. Due: "
                       << totalDue << ", paid: " << amount;
        return Unexpected(tecINSUFFICIENT_PAYMENT);
    }

    return late;
}

/* Handle possible full payments.
 *
 * If this function processed a full payment, the return value will be
 * a PaymentComponentsPlus object. If the payment should not be considered as a
 * full payment, the return will be an Unexpected(tesSUCCESS). Otherwise, it'll
 * be an Unexpected with the error code the caller is expected to return.
 */
template <AssetType A>
Expected<PaymentComponentsPlus, TER>
handleFullPayment(
    A const& asset,
    ApplyView& view,
    Number const& principalOutstanding,
    Number const& periodicPayment,
    std::uint32_t paymentRemaining,
    std::uint32_t prevPaymentDate,
    std::uint32_t const startDate,
    std::uint32_t const paymentInterval,
    TenthBips32 const closeInterestRate,
    std::int32_t loanScale,
    Number const& totalInterestOutstanding,
    Number const& periodicRate,
    Number const& closePaymentFee,
    STAmount const& amount,
    beast::Journal j)
{
    if (paymentRemaining <= 1)
        // If this is the last payment, it has to be a regular payment
        return Unexpected(tesSUCCESS);

    Number const rawPrincipalOutstanding = loanPrincipalFromPeriodicPayment(
        periodicPayment, periodicRate, paymentRemaining);

    auto const totalInterest = calculateFullPaymentInterest(
        asset,
        rawPrincipalOutstanding,
        periodicRate,
        view.parentCloseTime(),
        paymentInterval,
        prevPaymentDate,
        startDate,
        closeInterestRate,
        loanScale);

    auto const closeFullPayment =
        principalOutstanding + totalInterest + closePaymentFee;

    if (amount < closeFullPayment)
        // If the payment is less than the full payment amount, it's not
        // sufficient to be a full payment, but that's not an error.
        return Unexpected(tesSUCCESS);

    // Make a full payment

    PaymentComponentsPlus const result{
        PaymentComponents{
            .rawInterest =
                principalOutstanding + totalInterest - rawPrincipalOutstanding,
            .rawPrincipal = rawPrincipalOutstanding,
            .roundedInterest = totalInterest,
            .roundedPrincipal = principalOutstanding,
            .roundedPayment = principalOutstanding + totalInterest,
            .final = true},
        // A full payment only pays the single close payment fee
        closePaymentFee,
        // A full payment decreases the value of the loan by the
        // difference between the interest paid and the expected
        // outstanding interest return
        totalInterest - totalInterestOutstanding};

    return result;
}

}  // namespace detail

template <AssetType A>
Number
valueMinusFee(
    A const& asset,
    Number const& value,
    TenthBips32 managementFeeRate,
    std::int32_t scale)
{
    return roundToAsset(
        asset, detail::minusFee(value, managementFeeRate), scale);
}

struct LoanProperties
{
    Number periodicPayment;
    Number totalValueOutstanding;
    Number interestOwedToVault;
    std::int32_t loanScale;
    Number firstPaymentPrincipal;
};

template <AssetType A>
LoanProperties
computeLoanProperties(
    A const& asset,
    Number principalOutstanding,
    TenthBips32 interestRate,
    std::uint32_t paymentInterval,
    std::uint32_t paymentsRemaining,
    TenthBips32 managementFeeRate)
{
    auto const periodicRate = loanPeriodicRate(interestRate, paymentInterval);
    XRPL_ASSERT(
        interestRate == 0 || periodicRate > 0,
        "ripple::computeLoanProperties : valid rate");

    auto const periodicPayment = detail::loanPeriodicPayment(
        principalOutstanding, periodicRate, paymentsRemaining);
    Number const totalValueOutstanding = [&]() {
        NumberRoundModeGuard mg(Number::to_nearest);
        // Use STAmount's internal rounding instead of roundToAsset, because
        // we're going to use this result to determine the scale for all the
        // other rounding.
        return STAmount{
            asset,
            /*
             * This formula is from the XLS-66 spec, section 3.2.4.2 (Total
             * Loan Value Calculation), specifically "totalValueOutstanding
             * = ..."
             */
            periodicPayment * paymentsRemaining};
    }();
    // Base the loan scale on the total value, since that's going to be the
    // biggest number involved (barring unusual parameters for late, full, or
    // over payments)
    auto const loanScale = totalValueOutstanding.exponent();

    // Since we just figured out the loan scale, we haven't been able to
    // validate that the principal fits in it, so to allow this function to
    // succeed, round it here, and let the caller do the validation.
    principalOutstanding = roundToAsset(
        asset, principalOutstanding, loanScale, Number::to_nearest);

    auto const firstPaymentPrincipal = [&]() {
        // Compute the parts for the first payment. Ensure that the
        // principal payment will actually change the principal.
        auto const paymentComponents = detail::computePaymentComponents(
            asset,
            loanScale,
            totalValueOutstanding,
            principalOutstanding,
            periodicPayment,
            periodicRate,
            paymentsRemaining);

        // The unrounded principal part needs to be large enough to affect the
        // principal. What to do if not is left to the caller
        return paymentComponents.rawPrincipal;
    }();

    auto const interestOwedToVault = valueMinusFee(
        asset,
        /*
         * This formula is from the XLS-66 spec, section 3.2.4.2 (Total Loan
         * Value Calculation), specifically "totalInterestOutstanding = ..."
         */
        totalValueOutstanding - principalOutstanding,
        managementFeeRate,
        loanScale);

    return LoanProperties{
        .periodicPayment = periodicPayment,
        .totalValueOutstanding = totalValueOutstanding,
        .interestOwedToVault = interestOwedToVault,
        .loanScale = loanScale,
        .firstPaymentPrincipal = firstPaymentPrincipal};
}

template <AssetType A>
Number
calculateFullPaymentInterest(
    A const& asset,
    Number const& rawPrincipalOutstanding,
    Number const& periodicRate,
    NetClock::time_point parentCloseTime,
    std::uint32_t paymentInterval,
    std::uint32_t prevPaymentDate,
    std::uint32_t startDate,
    TenthBips32 closeInterestRate,
    std::int32_t loanScale)
{
    // If there is more than one payment remaining, see if enough was
    // paid for a full payment
    auto const accruedInterest = roundToAsset(
        asset,
        detail::loanAccruedInterest(
            rawPrincipalOutstanding,
            periodicRate,
            parentCloseTime,
            startDate,
            prevPaymentDate,
            paymentInterval),
        loanScale);
    XRPL_ASSERT(
        accruedInterest >= 0,
        "ripple::detail::handleFullPayment : valid accrued interest");

    auto const prepaymentPenalty = roundToAsset(
        asset,
        tenthBipsOfValue(rawPrincipalOutstanding, closeInterestRate),
        loanScale);
    XRPL_ASSERT(
        prepaymentPenalty >= 0,
        "ripple::detail::handleFullPayment : valid prepayment "
        "interest");
    return accruedInterest + prepaymentPenalty;
}

template <AssetType A>
Number
calculateFullPaymentInterest(
    A const& asset,
    Number const& periodicPayment,
    Number const& periodicRate,
    std::uint32_t paymentRemaining,
    NetClock::time_point parentCloseTime,
    std::uint32_t paymentInterval,
    std::uint32_t prevPaymentDate,
    std::uint32_t startDate,
    TenthBips32 closeInterestRate,
    std::int32_t loanScale)
{
    Number const rawPrincipalOutstanding =
        detail::loanPrincipalFromPeriodicPayment(
            periodicPayment, periodicRate, paymentRemaining);

    return calculateFullPaymentInterest(
        asset,
        rawPrincipalOutstanding,
        periodicRate,
        parentCloseTime,
        paymentInterval,
        prevPaymentDate,
        startDate,
        closeInterestRate,
        loanScale);
}

#if LOANCOMPLETE
template <AssetType A>
Number
loanPeriodicPayment(
    A const& asset,
    Number const& principalOutstanding,
    Number const& periodicRate,
    std::uint32_t paymentsRemaining,
    std::int32_t scale)
{
    return roundPeriodicPayment(
        asset,
        detail::loanPeriodicPayment(
            principalOutstanding, periodicRate, paymentsRemaining),
        scale);
}

template <AssetType A>
Number
loanPeriodicPayment(
    A const& asset,
    Number const& principalOutstanding,
    TenthBips32 interestRate,
    std::uint32_t paymentInterval,
    std::uint32_t paymentsRemaining,
    std::int32_t scale)
{
    return loanPeriodicPayment(
        asset,
        principalOutstanding,
        loanPeriodicRate(interestRate, paymentInterval),
        paymentsRemaining,
        scale);
}

template <AssetType A>
Number
loanTotalValueOutstanding(
    A asset,
    std::int32_t scale,
    Number const& periodicPayment,
    std::uint32_t paymentsRemaining)
{
    return roundToAsset(
        asset,
        /*
         * This formula is from the XLS-66 spec, section 3.2.4.2 (Total Loan
         * Value Calculation), specifically "totalValueOutstanding = ..."
         */
        periodicPayment * paymentsRemaining,
        scale,
        Number::upward);
}

template <AssetType A>
Number
loanTotalValueOutstanding(
    A asset,
    std::int32_t scale,
    Number const& principalOutstanding,
    TenthBips32 interestRate,
    std::uint32_t paymentInterval,
    std::uint32_t paymentsRemaining)
{
    /*
     * This function is derived from the XLS-66 spec, section 3.2.4.2 (Total
     * Loan Value Calculation)
     */
    return loanTotalValueOutstanding(
        asset,
        scale,
        loanPeriodicPayment(
            asset,
            principalOutstanding,
            interestRate,
            paymentInterval,
            paymentsRemaining,
            scale),
        paymentsRemaining);
}

inline Number
loanTotalInterestOutstanding(
    Number const& principalOutstanding,
    Number const& totalValueOutstanding)
{
    /*
     * This formula is from the XLS-66 spec, section 3.2.4.2 (Total Loan
     * Value Calculation), specifically "totalInterestOutstanding = ..."
     */
    return totalValueOutstanding - principalOutstanding;
}

template <AssetType A>
Number
loanTotalInterestOutstanding(
    A asset,
    std::int32_t scale,
    Number const& principalOutstanding,
    TenthBips32 interestRate,
    std::uint32_t paymentInterval,
    std::uint32_t paymentsRemaining)
{
    /*
     * This formula is derived from the XLS-66 spec, section 3.2.4.2 (Total Loan
     * Value Calculation)
     */
    return loanTotalInterestOutstanding(
        principalOutstanding,
        loanTotalValueOutstanding(
            asset,
            scale,
            principalOutstanding,
            interestRate,
            paymentInterval,
            paymentsRemaining));
}
#endif

template <AssetType A>
Number
loanInterestOutstandingMinusFee(
    A const& asset,
    Number const& totalInterestOutstanding,
    TenthBips32 managementFeeRate,
    std::int32_t scale)
{
    return valueMinusFee(
        asset, totalInterestOutstanding, managementFeeRate, scale);
}

#if LOANCOMPLETE
template <AssetType A>
Number
loanInterestOutstandingMinusFee(
    A const& asset,
    std::int32_t scale,
    Number const& principalOutstanding,
    TenthBips32 interestRate,
    std::uint32_t paymentInterval,
    std::uint32_t paymentsRemaining,
    TenthBips32 managementFeeRate)
{
    return loanInterestOutstandingMinusFee(
        asset,
        loanTotalInterestOutstanding(
            asset,
            scale,
            principalOutstanding,
            interestRate,
            paymentInterval,
            paymentsRemaining),
        managementFeeRate,
        scale);
}
#endif

template <AssetType A>
Number
loanLatePaymentInterest(
    A const& asset,
    Number const& principalOutstanding,
    TenthBips32 lateInterestRate,
    NetClock::time_point parentCloseTime,
    std::uint32_t nextPaymentDueDate,
    std::int32_t const& scale)
{
    return roundToAsset(
        asset,
        detail::loanLatePaymentInterest(
            principalOutstanding,
            lateInterestRate,
            parentCloseTime,
            nextPaymentDueDate),
        scale);
}

template <AssetType A>
bool
isRounded(A const& asset, Number const& value, std::int32_t scale)
{
    return roundToAsset(asset, value, scale, Number::downward) ==
        roundToAsset(asset, value, scale, Number::upward);
}

template <AssetType A>
Expected<LoanPaymentParts, TER>
loanMakePayment(
    A const& asset,
    ApplyView& view,
    SLE::ref loan,
    STAmount const& amount,
    TenthBips16 managementFeeRate,
    beast::Journal j)
{
    /*
     * This function is an implementation of the XLS-66 spec,
     * section 3.2.4.3 (Transaction Pseudo-code)
     */
    auto principalOutstandingProxy = loan->at(sfPrincipalOutstanding);
    auto paymentRemainingProxy = loan->at(sfPaymentRemaining);

    if (paymentRemainingProxy == 0 || principalOutstandingProxy == 0)
    {
        // Loan complete
        JLOG(j.warn()) << "Loan is already paid off.";
        return Unexpected(tecKILLED);
    }

    // Next payment due date must be set unless the loan is complete
    auto nextDueDateProxy = loan->at(~sfNextPaymentDueDate);
    if (!nextDueDateProxy)
    {
        JLOG(j.warn()) << "Loan next payment due date is not set.";
        return Unexpected(tecINTERNAL);
    }

    std::int32_t const loanScale = loan->at(sfLoanScale);
    auto totalValueOutstandingProxy = loan->at(sfTotalValueOutstanding);

    TenthBips32 const interestRate{loan->at(sfInterestRate)};
    TenthBips32 const lateInterestRate{loan->at(sfLateInterestRate)};
    TenthBips32 const closeInterestRate{loan->at(sfCloseInterestRate)};

    Number const serviceFee = loan->at(sfLoanServiceFee);
    Number const latePaymentFee = loan->at(sfLatePaymentFee);
    Number const closePaymentFee =
        roundToAsset(asset, loan->at(sfClosePaymentFee), loanScale);

    auto const periodicPayment = loan->at(sfPeriodicPayment);

    auto prevPaymentDateProxy = loan->at(sfPreviousPaymentDate);
    std::uint32_t const startDate = loan->at(sfStartDate);

    std::uint32_t const paymentInterval = loan->at(sfPaymentInterval);
    // Compute the normal periodic rate, payment, etc.
    // We'll need it in the remaining calculations
    Number const periodicRate = loanPeriodicRate(interestRate, paymentInterval);
    XRPL_ASSERT(
        interestRate == 0 || periodicRate > 0,
        "ripple::loanMakePayment : valid rate");

    XRPL_ASSERT(
        *totalValueOutstandingProxy > 0,
        "ripple::loanMakePayment : valid total value");

    view.update(loan);

    detail::PaymentComponentsPlus const periodic{
        detail::computePaymentComponents(
            asset,
            loanScale,
            totalValueOutstandingProxy,
            principalOutstandingProxy,
            periodicPayment,
            periodicRate,
            paymentRemainingProxy),
        serviceFee};
    XRPL_ASSERT_PARTS(
        periodic.roundedPrincipal > 0,
        "ripple::loanMakePayment",
        "regular payment pays principal");

    // -------------------------------------------------------------
    // late payment handling
    if (auto const latePaymentComponents = detail::handleLatePayment(
            asset,
            view,
            principalOutstandingProxy,
            *nextDueDateProxy,
            periodic,
            lateInterestRate,
            loanScale,
            latePaymentFee,
            amount,
            j))
    {
        return doPayment(
            *latePaymentComponents,
            totalValueOutstandingProxy,
            principalOutstandingProxy,
            paymentRemainingProxy,
            prevPaymentDateProxy,
            nextDueDateProxy,
            paymentInterval);
    }
    else if (latePaymentComponents.error())
        // error() will be the TER returned if a payment is not made. It will
        // only evaluate to true if it's unsuccessful. Otherwise, tesSUCCESS
        // means nothing was done, so continue.
        return Unexpected(latePaymentComponents.error());

    // -------------------------------------------------------------
    // full payment handling
    auto const totalInterestOutstanding =
        totalValueOutstandingProxy - principalOutstandingProxy;

    if (auto const fullPaymentComponents = detail::handleFullPayment(
            asset,
            view,
            principalOutstandingProxy,
            periodicPayment,
            paymentRemainingProxy,
            prevPaymentDateProxy,
            startDate,
            paymentInterval,
            closeInterestRate,
            loanScale,
            totalInterestOutstanding,
            periodicRate,
            closePaymentFee,
            amount,
            j))
        return doPayment(
            *fullPaymentComponents,
            totalValueOutstandingProxy,
            principalOutstandingProxy,
            paymentRemainingProxy,
            prevPaymentDateProxy,
            nextDueDateProxy,
            paymentInterval);
    else if (fullPaymentComponents.error())
        // error() will be the TER returned if a payment is not made. It will
        // only evaluate to true if it's unsuccessful. Otherwise, tesSUCCESS
        // means nothing was done, so continue.
        return Unexpected(fullPaymentComponents.error());

    // -------------------------------------------------------------
    // regular periodic payment handling

    // if the payment is not late nor if it's a full payment, then it must
    // be a periodic one, with possible overpayments

    // This will keep a running total of what is actually paid, if the payment
    // is sufficient for a single payment
    Number totalPaid =
        periodic.roundedInterest + periodic.roundedPrincipal + periodic.fee;

    if (amount < totalPaid)
    {
        JLOG(j.warn()) << "Periodic loan payment amount is insufficient. Due: "
                       << totalPaid << ", paid: " << amount;
        return Unexpected(tecINSUFFICIENT_PAYMENT);
    }

    LoanPaymentParts totalParts = detail::doPayment(
        periodic,
        totalValueOutstandingProxy,
        principalOutstandingProxy,
        paymentRemainingProxy,
        prevPaymentDateProxy,
        nextDueDateProxy,
        paymentInterval);

    std::size_t numPayments = 1;

    while (totalPaid < amount && paymentRemainingProxy > 0)
    {
        // Try to make more payments
        detail::PaymentComponentsPlus const nextPayment{
            detail::computePaymentComponents(
                asset,
                loanScale,
                totalValueOutstandingProxy,
                principalOutstandingProxy,
                periodicPayment,
                periodicRate,
                paymentRemainingProxy),
            periodic.fee};
        XRPL_ASSERT_PARTS(
            nextPayment.roundedPrincipal > 0,
            "ripple::loanMakePayment",
            "additional payment pays principal");
        XRPL_ASSERT(
            nextPayment.rawInterest <= periodic.rawInterest,
            "ripple::loanMakePayment : decreasing interest");
        XRPL_ASSERT(
            nextPayment.rawPrincipal >= periodic.rawPrincipal,
            "ripple::loanMakePayment : increasing principal");

        // the fee part doesn't change
        auto const due = nextPayment.roundedInterest +
            nextPayment.roundedPrincipal + periodic.fee;

        if (amount < totalPaid + due)
            // We're done making payments.
            break;

        totalPaid += due;
        totalParts += detail::doPayment(
            nextPayment,
            totalValueOutstandingProxy,
            principalOutstandingProxy,
            paymentRemainingProxy,
            prevPaymentDateProxy,
            nextDueDateProxy,
            paymentInterval);
        ++numPayments;

        if (nextPayment.final)
            break;
    }

    XRPL_ASSERT_PARTS(
        totalParts.principalPaid + totalParts.interestPaid +
                totalParts.feeToPay ==
            totalPaid,
        "ripple::loanMakePayment",
        "payment parts add up");
    XRPL_ASSERT_PARTS(
        totalParts.valueChange == 0,
        "ripple::loanMakePayment",
        "no value change");
    XRPL_ASSERT_PARTS(
        totalParts.feeToPay == periodic.fee * numPayments,
        "ripple::loanMakePayment",
        "fee parts add up");

    // -------------------------------------------------------------
    // overpayment handling
    if (loan->isFlag(lsfLoanOverpayment) && paymentRemainingProxy > 0 &&
        nextDueDateProxy && totalPaid < amount)
    {
        TenthBips32 const overpaymentInterestRate{
            loan->at(sfOverpaymentInterestRate)};
        TenthBips32 const overpaymentFeeRate{loan->at(sfOverpaymentFee)};

        Number const overpayment = amount - totalPaid;
        XRPL_ASSERT(
            overpayment > 0 && isRounded(asset, overpayment, loanScale),
            "ripple::loanMakePayment : valid overpayment amount");

        Number const fee = roundToAsset(
            asset,
            tenthBipsOfValue(overpayment, overpaymentFeeRate),
            loanScale);

        Number const payment = overpayment - fee;

        // TODO: Is the overpaymentInterestRate an APR or flat?

        Number const interest =
            tenthBipsOfValue(payment, overpaymentInterestRate);
        Number const roundedInterest = roundToAsset(asset, interest, loanScale);

        detail::PaymentComponentsPlus overpaymentComponents{
            PaymentComponents{
                .rawInterest = interest,
                .rawPrincipal = payment - interest,
                .roundedInterest = roundedInterest,
                .roundedPrincipal = payment - roundedInterest,
                .roundedPayment = payment,
                .extra = true},
            fee,
            roundedInterest};

        // Don't process an overpayment if the whole amount (or more!)
        // gets eaten by fees and interest.
        if (overpaymentComponents.rawPrincipal > 0 &&
            overpaymentComponents.roundedPrincipal > 0)
        {
            auto periodicPaymentProxy = loan->at(sfPeriodicPayment);
            if (auto const overResult = detail::doOverpayment(
                    asset,
                    view,
                    overpaymentComponents,
                    totalValueOutstandingProxy,
                    principalOutstandingProxy,
                    periodicPaymentProxy,
                    interestRate,
                    paymentInterval,
                    paymentRemainingProxy,
                    prevPaymentDateProxy,
                    nextDueDateProxy,
                    managementFeeRate,
                    j))
                totalParts += *overResult;
            else if (overResult.error())
                // error() will be the TER returned if a payment is not made. It
                // will only evaluate to true if it's unsuccessful. Otherwise,
                // tesSUCCESS means nothing was done, so continue.
                return Unexpected(overResult.error());
        }
    }

    // Check the final results are rounded, to double-check that the
    // intermediate steps were rounded.
    XRPL_ASSERT(
        isRounded(asset, totalParts.principalPaid, loanScale),
        "ripple::loanMakePayment : total principal paid rounded");
    XRPL_ASSERT(
        isRounded(asset, totalParts.interestPaid, loanScale),
        "ripple::loanMakePayment : total interest paid rounded");
    XRPL_ASSERT(
        isRounded(asset, totalParts.valueChange, loanScale),
        "ripple::loanMakePayment : loan value change rounded");
    XRPL_ASSERT(
        isRounded(asset, totalParts.feeToPay, loanScale),
        "ripple::loanMakePayment : total fee to pay rounded");
    return totalParts;
}

}  // namespace ripple

#endif  // RIPPLE_APP_MISC_LENDINGHELPERS_H_INCLUDED
