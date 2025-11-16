#include <xrpl/basics/Number.h>
#include <xrpl/basics/contract.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/protocol/Quality.h>
#include <xrpl/protocol/QualityFunction.h>

#include <optional>
#include <stdexcept>

namespace ripple {

QualityFunction::QualityFunction(
    Quality const& quality,
    QualityFunction::CLOBLikeTag)
    : m_(0), b_(0), quality_(quality)
{
    if (quality.rate() <= beast::zero)
        Throw<std::runtime_error>("QualityFunction quality rate is 0.");

    static_assert(std::is_arithmetic_v<std::remove_reference_t<int>>);
    static_assert(std::is_convertible_v<ripple::STAmount, Number>);
    static_assert(ripple::OneNumberParam<int, ripple::STAmount>);
    static_assert(!ripple::OneNumberParam<Number, Number>);

    b_ = 1 / quality.rate();
}

void
QualityFunction::combine(QualityFunction const& qf)
{
    m_ += b_ * qf.m_;
    b_ *= qf.b_;
    if (m_ != 0)
        quality_ = std::nullopt;
}

std::optional<Number>
QualityFunction::outFromAvgQ(Quality const& quality)
{
    if (m_ != 0 && quality.rate() != beast::zero)
    {
        saveNumberRoundMode rm(Number::setround(Number::rounding_mode::upward));
        auto const out = (1 / quality.rate() - b_) / m_;
        if (out <= 0)
            return std::nullopt;
        return out;
    }
    return std::nullopt;
}

}  // namespace ripple
