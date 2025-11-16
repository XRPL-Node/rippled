#include <xrpl/basics/Number.h>
#include <xrpl/beast/core/LexicalCast.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STBase.h>
#include <xrpl/protocol/STNumber.h>
#include <xrpl/protocol/Serializer.h>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <cstddef>
#include <ostream>
#include <string>
#include <utility>

namespace ripple {

STNumber::STNumber(SField const& field, Number const& value)
    : STBase(field), value_(value)
{
}

STNumber::STNumber(SerialIter& sit, SField const& field) : STBase(field)
{
    // We must call these methods in separate statements
    // to guarantee their order of execution.
    auto mantissa = sit.geti64();
    auto exponent = sit.geti32();
    value_ = Number{mantissa, exponent};
}

SerializedTypeID
STNumber::getSType() const
{
    return STI_NUMBER;
}

std::string
STNumber::getText() const
{
    return to_string(value_);
}

void
STNumber::add(Serializer& s) const
{
    XRPL_ASSERT(
        getFName().isBinary(), "ripple::STNumber::add : field is binary");
    XRPL_ASSERT(
        getFName().fieldType == getSType(),
        "ripple::STNumber::add : field type match");
    if (!validNumber())
        throw NumberOverflow(to_string(value_));
    s.add64(value_.mantissa());
    s.add32(value_.exponent());
}

Number const&
STNumber::value() const
{
    return value_;
}

void
STNumber::setValue(Number const& v)
{
    value_ = v;
}

// Tell the STNumber whether the value it is holding represents an integer, and
// must fit within the allowable range.
void
STNumber::usesAsset(Asset const& a)
{
    XRPL_ASSERT_PARTS(
        !isInteger_ || a.integral(),
        "ripple::STNumber::value",
        "asset check only gets stricter");
    // isInteger_ is a one-way switch. Once it's on, it stays on.
    if (isInteger_)
        return;
    isInteger_ = a.integral();
}

bool
STNumber::isIntegral() const
{
    return isInteger_;
}

// Returns whether the value fits within Number::maxIntValue. Transactors
// should check this whenever interacting with an STNumber.
bool
STNumber::safeNumber() const
{
    if (!isInteger_)
        return true;

    static Number const max = safeNumberLimit();
    static Number const maxNeg = -max;
    // Avoid making a copy
    if (value_ < 0)
        return value_ >= maxNeg;
    return value_ <= max;
}

bool
STNumber::safeNumber(Asset const& a)
{
    usesAsset(a);
    return safeNumber();
}

std::int64_t
STNumber::safeNumberLimit()
{
    return Number::maxIntValue;
}

// Returns whether the value fits within Number::maxMantissa. Transactors
// may check this, too, but are not required to. It will be checked when
// serializing, and will throw if false, thus preventing the value from
// being silently truncated.
bool
STNumber::validNumber() const
{
    if (!isInteger_)
        return true;

    static Number const max = validNumberLimit();
    static Number const maxNeg = -max;
    // Avoid making a copy
    if (value_ < 0)
        return value_ >= maxNeg;
    return value_ <= max;
}

bool
STNumber::validNumber(Asset const& a)
{
    usesAsset(a);
    return validNumber();
}

std::int64_t
STNumber::validNumberLimit()
{
    return Number::maxMantissa;
}

STBase*
STNumber::copy(std::size_t n, void* buf) const
{
    return emplace(n, buf, *this);
}

STBase*
STNumber::move(std::size_t n, void* buf)
{
    return emplace(n, buf, std::move(*this));
}

bool
STNumber::isEquivalent(STBase const& t) const
{
    XRPL_ASSERT(
        t.getSType() == this->getSType(),
        "ripple::STNumber::isEquivalent : field type match");
    STNumber const& v = dynamic_cast<STNumber const&>(t);
    return value_ == v;
}

bool
STNumber::isDefault() const
{
    return value_ == Number();
}

std::ostream&
operator<<(std::ostream& out, STNumber const& rhs)
{
    return out << rhs.getText();
}

NumberParts
partsFromString(std::string const& number)
{
    static boost::regex const reNumber(
        "^"                       // the beginning of the string
        "([-+]?)"                 // (optional) + or - character
        "(0|[1-9][0-9]*)"         // a number (no leading zeroes, unless 0)
        "(\\.([0-9]+))?"          // (optional) period followed by any number
        "([eE]([+-]?)([0-9]+))?"  // (optional) E, optional + or -, any number
        "$",
        boost::regex_constants::optimize);

    boost::smatch match;

    if (!boost::regex_match(number, match, reNumber))
        Throw<std::runtime_error>("'" + number + "' is not a number");

    // Match fields:
    //   0 = whole input
    //   1 = sign
    //   2 = integer portion
    //   3 = whole fraction (with '.')
    //   4 = fraction (without '.')
    //   5 = whole exponent (with 'e')
    //   6 = exponent sign
    //   7 = exponent number

    bool negative = (match[1].matched && (match[1] == "-"));

    std::uint64_t mantissa;
    int exponent;

    if (!match[4].matched)  // integer only
    {
        mantissa = boost::lexical_cast<std::uint64_t>(std::string(match[2]));
        exponent = 0;
    }
    else
    {
        // integer and fraction
        mantissa = boost::lexical_cast<std::uint64_t>(match[2] + match[4]);
        exponent = -(match[4].length());
    }

    if (match[5].matched)
    {
        // we have an exponent
        if (match[6].matched && (match[6] == "-"))
            exponent -= boost::lexical_cast<int>(std::string(match[7]));
        else
            exponent += boost::lexical_cast<int>(std::string(match[7]));
    }

    return {mantissa, exponent, negative};
}

STNumber
numberFromJson(SField const& field, Json::Value const& value)
{
    NumberParts parts;

    if (value.isInt())
    {
        if (value.asInt() >= 0)
        {
            parts.mantissa = value.asInt();
        }
        else
        {
            parts.mantissa = value.asAbsUInt();
            parts.negative = true;
        }
    }
    else if (value.isUInt())
    {
        parts.mantissa = value.asUInt();
    }
    else if (value.isString())
    {
        parts = partsFromString(value.asString());
        // Only strings can represent out-of-range values.
        if (parts.mantissa > std::numeric_limits<std::int64_t>::max())
            Throw<std::range_error>("too high");
    }
    else
    {
        Throw<std::runtime_error>("not a number");
    }

    std::int64_t mantissa = parts.mantissa;
    if (parts.negative)
        mantissa = -mantissa;

    return STNumber{field, Number{mantissa, parts.exponent}};
}

}  // namespace ripple
