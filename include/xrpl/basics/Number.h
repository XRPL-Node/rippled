#ifndef XRPL_BASICS_NUMBER_H_INCLUDED
#define XRPL_BASICS_NUMBER_H_INCLUDED

#include <xrpl/beast/utility/instrumentation.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace ripple {

class Number;

std::string
to_string(Number const& amount);

template <typename T>
constexpr std::optional<int>
logTen(T value)
{
    int log = 0;
    while (value >= 10 && value % 10 == 0)
    {
        value /= 10;
        ++log;
    }
    if (value == 1)
        return log;
    return std::nullopt;
}

template <typename T>
constexpr bool
isPowerOfTen(T value)
{
    return logTen(value).has_value();
}

struct MantissaRange
{
    using rep = std::uint64_t;
    enum mantissa_scale { small, large };

    explicit constexpr MantissaRange(mantissa_scale scale_, rep min_)
        : min(min_)
        , max(min_ * 10 - 1)
        , log(logTen(min).value_or(-1))
        , scale(scale_)
    {
    }

    rep min;
    rep max;
    int log;
    mantissa_scale scale;
};

class Number
{
    using rep = std::int64_t;
    using internalrep = MantissaRange::rep;

    bool negative_{false};
    internalrep mantissa_{0};
    int exponent_{std::numeric_limits<int>::lowest()};

public:
    // The range for the exponent when normalized
    constexpr static int minExponent = -32768;
    constexpr static int maxExponent = 32768;

    // May need to make unchecked private
    struct unchecked
    {
        explicit unchecked() = default;
    };

    // Like unchecked, normalized is used with the ctors that take an
    // internalrep mantissa. Unlike unchecked, those ctors will normalize the
    // value.
    // Only unit tests are expected to use this class
    struct normalized
    {
        explicit normalized() = default;
    };

    explicit constexpr Number() = default;

    Number(rep mantissa);
    explicit Number(rep mantissa, int exponent);
    explicit constexpr Number(
        bool negative,
        internalrep mantissa,
        int exponent,
        unchecked) noexcept;
    // Assume unsigned values are... unsigned. i.e. positive
    explicit constexpr Number(
        internalrep mantissa,
        int exponent,
        unchecked) noexcept;
    // Only unit tests are expected to use this ctor
    explicit Number(
        bool negative,
        internalrep mantissa,
        int exponent,
        normalized);
    // Assume unsigned values are... unsigned. i.e. positive
    explicit Number(internalrep mantissa, int exponent, normalized);

    constexpr rep
    mantissa() const noexcept;
    constexpr int
    exponent() const noexcept;

    constexpr Number
    operator+() const noexcept;
    constexpr Number
    operator-() const noexcept;
    Number&
    operator++();
    Number
    operator++(int);
    Number&
    operator--();
    Number
    operator--(int);

    Number&
    operator+=(Number const& x);
    Number&
    operator-=(Number const& x);

    Number&
    operator*=(Number const& x);
    Number&
    operator/=(Number const& x);

    static Number
    min() noexcept;
    static Number
    max() noexcept;
    static Number
    lowest() noexcept;

    /** Conversions to Number are implicit and conversions away from Number
     *  are explicit. This design encourages and facilitates the use of Number
     *  as the preferred type for floating point arithmetic as it makes
     *  "mixed mode" more convenient, e.g. MPTAmount + Number.
     */
    explicit
    operator rep() const;  // round to nearest, even on tie

    friend constexpr bool
    operator==(Number const& x, Number const& y) noexcept
    {
        return x.negative_ == y.negative_ && x.mantissa_ == y.mantissa_ &&
            x.exponent_ == y.exponent_;
    }

    friend constexpr bool
    operator!=(Number const& x, Number const& y) noexcept
    {
        return !(x == y);
    }

    friend constexpr bool
    operator<(Number const& x, Number const& y) noexcept
    {
        // If the two amounts have different signs (zero is treated as positive)
        // then the comparison is true iff the left is negative.
        bool const lneg = x.negative_;
        bool const rneg = y.negative_;

        if (lneg != rneg)
            return lneg;

        // Both have same sign and the left is zero: the right must be
        // greater than 0.
        if (x.mantissa_ == 0)
            return y.mantissa_ > 0;

        // Both have same sign, the right is zero and the left is non-zero.
        if (y.mantissa_ == 0)
            return false;

        // Both have the same sign, compare by exponents:
        if (x.exponent_ > y.exponent_)
            return lneg;
        if (x.exponent_ < y.exponent_)
            return !lneg;

        // If equal exponents, compare mantissas
        return x.mantissa_ < y.mantissa_;
    }

    /** Return the sign of the amount */
    constexpr int
    signum() const noexcept
    {
        return negative_ ? -1 : (mantissa_ ? 1 : 0);
    }

    Number
    truncate() const noexcept;

    friend constexpr bool
    operator>(Number const& x, Number const& y) noexcept
    {
        return y < x;
    }

    friend constexpr bool
    operator<=(Number const& x, Number const& y) noexcept
    {
        return !(y < x);
    }

    friend constexpr bool
    operator>=(Number const& x, Number const& y) noexcept
    {
        return !(x < y);
    }

    friend std::ostream&
    operator<<(std::ostream& os, Number const& x)
    {
        return os << to_string(x);
    }

    friend std::string
    to_string(Number const& amount);

    friend Number
    root(Number f, unsigned d);

    friend Number
    root2(Number f);

    // Thread local rounding control.  Default is to_nearest
    enum rounding_mode { to_nearest, towards_zero, downward, upward };
    static rounding_mode
    getround();
    // Returns previously set mode
    static rounding_mode
    setround(rounding_mode mode);

    static MantissaRange::mantissa_scale
    getMantissaScale();
    static void
    setMantissaScale(MantissaRange::mantissa_scale scale);

    inline static internalrep
    minMantissa()
    {
        return range_.get().min;
    }

    inline static internalrep
    maxMantissa()
    {
        return range_.get().max;
    }

    inline static int
    mantissaLog()
    {
        return range_.get().log;
    }

    /// oneSmall is needed because the ranges are private
    constexpr static Number
    oneSmall();
    /// oneLarge is needed because the ranges are private
    constexpr static Number
    oneLarge();

    // And one is needed because it needs to choose between oneSmall and
    // oneLarge based on the current range
    static Number
    one();

    template <class T>
    [[nodiscard]]
    std::pair<T, int>
    normalizeToRange(T minMantissa, T maxMantissa) const;

private:
    static thread_local rounding_mode mode_;
    // The available ranges for mantissa

    constexpr static internalrep maxRep = std::numeric_limits<rep>::max();
    static_assert(maxRep == 9'223'372'036'854'775'807);

    constexpr static MantissaRange smallRange{
        MantissaRange::small,
        1'000'000'000'000'000LL};
    static_assert(isPowerOfTen(smallRange.min));
    static_assert(smallRange.max == 9'999'999'999'999'999LL);
    static_assert(smallRange.log == 15);
    static_assert(smallRange.min < maxRep);
    static_assert(smallRange.max < maxRep);
    constexpr static MantissaRange largeRange{
        MantissaRange::large,
        1'000'000'000'000'000'000LL};
    static_assert(isPowerOfTen(largeRange.min));
    static_assert(largeRange.max == internalrep(9'999'999'999'999'999'999ULL));
    static_assert(largeRange.log == 18);
    static_assert(largeRange.min < maxRep);
    static_assert(largeRange.max > maxRep);

    // The range for the mantissa when normalized.
    // Use reference_wrapper to avoid making copies, and prevent accidentally
    // changing the values inside the range.
    static thread_local std::reference_wrapper<MantissaRange const> range_;

    void
    normalize();

    template <class T>
    static void
    normalize(
        bool& negative,
        T& mantissa,
        int& exponent,
        internalrep const& minMantissa,
        internalrep const& maxMantissa);

    bool
    isnormal() const noexcept;

    // Copy the number, but modify the exponent by "exponentDelta". Because the
    // mantissa doesn't change, the result will be "mostly" normalized, but the
    // exponent could go out of range, so it will be checked.
    Number
    shiftExponent(int exponentDelta) const;

    // Safely convert rep (int64) mantissa to internalrep (uint64). If the rep
    // is negative, returns the positive value. This takes a little extra work
    // because converting std::numeric_limits<std::int64_t>::min() flirts with
    // UB, and can vary across compilers.
    static internalrep
    externalToInternal(rep mantissa);

    class Guard;
};

inline constexpr Number::Number(
    bool negative,
    internalrep mantissa,
    int exponent,
    unchecked) noexcept
    : negative_(negative), mantissa_{mantissa}, exponent_{exponent}
{
}

inline constexpr Number::Number(
    internalrep mantissa,
    int exponent,
    unchecked) noexcept
    : Number(false, mantissa, exponent, unchecked{})
{
}

constexpr static Number numZero{};

inline Number::Number(
    bool negative,
    internalrep mantissa,
    int exponent,
    normalized)
    : Number(negative, mantissa, exponent, unchecked{})
{
    normalize();
}

inline Number::Number(internalrep mantissa, int exponent, normalized)
    : Number(false, mantissa, exponent, normalized{})
{
}

inline Number::Number(rep mantissa, int exponent)
    : Number(mantissa < 0, externalToInternal(mantissa), exponent, normalized{})
{
}

inline Number::Number(rep mantissa) : Number{mantissa, 0}
{
}

inline constexpr Number::rep
Number::mantissa() const noexcept
{
    auto m = mantissa_;
    if (m > maxRep)
    {
        XRPL_ASSERT_PARTS(
            !isnormal() || m % 10 == 0,
            "ripple::Number::mantissa",
            "large normalized mantissa has no remainder");
        m /= 10;
    }
    auto const sign = negative_ ? -1 : 1;
    return sign * static_cast<Number::rep>(m);
}

inline constexpr int
Number::exponent() const noexcept
{
    auto m = mantissa_;
    auto e = exponent_;
    if (m > maxRep)
    {
        XRPL_ASSERT_PARTS(
            !isnormal() || m % 10 == 0,
            "ripple::Number::exponent",
            "large normalized mantissa has no remainder");
        m /= 10;
        ++e;
    }
    return e;
}

inline constexpr Number
Number::operator+() const noexcept
{
    return *this;
}

inline constexpr Number
Number::operator-() const noexcept
{
    if (mantissa_ == 0)
        return Number{};
    auto x = *this;
    x.negative_ = !x.negative_;
    return x;
}

inline Number&
Number::operator++()
{
    *this += one();
    return *this;
}

inline Number
Number::operator++(int)
{
    auto x = *this;
    ++(*this);
    return x;
}

inline Number&
Number::operator--()
{
    *this -= one();
    return *this;
}

inline Number
Number::operator--(int)
{
    auto x = *this;
    --(*this);
    return x;
}

inline Number&
Number::operator-=(Number const& x)
{
    return *this += -x;
}

inline Number
operator+(Number const& x, Number const& y)
{
    auto z = x;
    z += y;
    return z;
}

inline Number
operator-(Number const& x, Number const& y)
{
    auto z = x;
    z -= y;
    return z;
}

inline Number
operator*(Number const& x, Number const& y)
{
    auto z = x;
    z *= y;
    return z;
}

inline Number
operator/(Number const& x, Number const& y)
{
    auto z = x;
    z /= y;
    return z;
}

inline Number
Number::min() noexcept
{
    return Number{false, range_.get().min, minExponent, unchecked{}};
}

inline Number
Number::max() noexcept
{
    return Number{
        false, std::min(range_.get().max, maxRep), maxExponent, unchecked{}};
}

inline Number
Number::lowest() noexcept
{
    return Number{
        true, std::min(range_.get().max, maxRep), maxExponent, unchecked{}};
}

inline bool
Number::isnormal() const noexcept
{
    MantissaRange const& range = range_;
    auto const abs_m = mantissa_;
    return *this == Number{} ||
        (range.min <= abs_m && abs_m <= range.max &&
         (abs_m <= maxRep || abs_m % 10 == 0) && minExponent <= exponent_ &&
         exponent_ <= maxExponent);
}

template <class T>
std::pair<T, int>
Number::normalizeToRange(T minMantissa, T maxMantissa) const
{
    bool negative = negative_;
    internalrep mantissa = mantissa_;
    int exponent = exponent_;
    Number::normalize(negative, mantissa, exponent, minMantissa, maxMantissa);

    auto const sign = negative ? -1 : 1;
    return std::make_pair(static_cast<T>(sign * mantissa), exponent);
}

inline constexpr Number
abs(Number x) noexcept
{
    if (x < Number{})
        x = -x;
    return x;
}

// Returns f^n
// Uses a log_2(n) number of multiplications

Number
power(Number const& f, unsigned n);

// Returns f^(1/d)
// Uses Newton–Raphson iterations until the result stops changing
// to find the root of the polynomial g(x) = x^d - f

Number
root(Number f, unsigned d);

Number
root2(Number f);

// Returns f^(n/d)

Number
power(Number const& f, unsigned n, unsigned d);

// Return 0 if abs(x) < limit, else returns x

inline constexpr Number
squelch(Number const& x, Number const& limit) noexcept
{
    if (abs(x) < limit)
        return Number{};
    return x;
}

inline std::string
to_string(MantissaRange::mantissa_scale const& scale)
{
    switch (scale)
    {
        case MantissaRange::small:
            return "small";
        case MantissaRange::large:
            return "large";
        default:
            throw std::runtime_error("Bad scale");
    }
}

class saveNumberRoundMode
{
    Number::rounding_mode mode_;

public:
    ~saveNumberRoundMode()
    {
        Number::setround(mode_);
    }
    explicit saveNumberRoundMode(Number::rounding_mode mode) noexcept
        : mode_{mode}
    {
    }
    saveNumberRoundMode(saveNumberRoundMode const&) = delete;
    saveNumberRoundMode&
    operator=(saveNumberRoundMode const&) = delete;
};

// saveNumberRoundMode doesn't do quite enough for us.  What we want is a
// Number::RoundModeGuard that sets the new mode and restores the old mode
// when it leaves scope.  Since Number doesn't have that facility, we'll
// build it here.
class NumberRoundModeGuard
{
    saveNumberRoundMode saved_;

public:
    explicit NumberRoundModeGuard(Number::rounding_mode mode) noexcept
        : saved_{Number::setround(mode)}
    {
    }

    NumberRoundModeGuard(NumberRoundModeGuard const&) = delete;

    NumberRoundModeGuard&
    operator=(NumberRoundModeGuard const&) = delete;
};

// Sets the new scale and restores the old scale when it leaves scope.  Since
// Number doesn't have that facility, we'll build it here.
//
// This class may only end up needed in tests
class NumberMantissaScaleGuard
{
    MantissaRange::mantissa_scale const saved_;

public:
    explicit NumberMantissaScaleGuard(
        MantissaRange::mantissa_scale scale) noexcept
        : saved_{Number::getMantissaScale()}
    {
        Number::setMantissaScale(scale);
    }

    ~NumberMantissaScaleGuard()
    {
        Number::setMantissaScale(saved_);
    }

    NumberMantissaScaleGuard(NumberMantissaScaleGuard const&) = delete;

    NumberMantissaScaleGuard&
    operator=(NumberMantissaScaleGuard const&) = delete;
};

}  // namespace ripple

#endif  // XRPL_BASICS_NUMBER_H_INCLUDED
