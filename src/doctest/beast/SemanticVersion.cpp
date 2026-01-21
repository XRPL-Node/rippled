#include <xrpl/beast/core/SemanticVersion.h>

#include <doctest/doctest.h>

using namespace beast;

TEST_SUITE_BEGIN("SemanticVersion");

using identifier_list = SemanticVersion::identifier_list;

namespace {

void
checkPass(std::string const& input, bool shouldPass = true)
{
    SemanticVersion v;

    if (shouldPass)
    {
        CHECK_UNARY(v.parse(input));
        CHECK_EQ(v.print(), input);
    }
    else
    {
        CHECK_FALSE(v.parse(input));
    }
}

void
checkFail(std::string const& input)
{
    checkPass(input, false);
}

// check input and input with appended metadata
void
checkMeta(std::string const& input, bool shouldPass)
{
    checkPass(input, shouldPass);

    checkPass(input + "+a", shouldPass);
    checkPass(input + "+1", shouldPass);
    checkPass(input + "+a.b", shouldPass);
    checkPass(input + "+ab.cd", shouldPass);

    checkFail(input + "!");
    checkFail(input + "+");
    checkFail(input + "++");
    checkFail(input + "+!");
    checkFail(input + "+.");
    checkFail(input + "+a.!");
}

void
checkMetaFail(std::string const& input)
{
    checkMeta(input, false);
}

// check input, input with appended release data,
// input with appended metadata, and input with both
// appended release data and appended metadata
void
checkRelease(std::string const& input, bool shouldPass = true)
{
    checkMeta(input, shouldPass);

    checkMeta(input + "-1", shouldPass);
    checkMeta(input + "-a", shouldPass);
    checkMeta(input + "-a1", shouldPass);
    checkMeta(input + "-a1.b1", shouldPass);
    checkMeta(input + "-ab.cd", shouldPass);
    checkMeta(input + "--", shouldPass);

    checkMetaFail(input + "+");
    checkMetaFail(input + "!");
    checkMetaFail(input + "-");
    checkMetaFail(input + "-!");
    checkMetaFail(input + "-.");
    checkMetaFail(input + "-a.!");
    checkMetaFail(input + "-0.a");
}

// Checks the major.minor.version string alone and with all
// possible combinations of release identifiers and metadata.
void
check(std::string const& input, bool shouldPass = true)
{
    checkRelease(input, shouldPass);
}

void
negcheck(std::string const& input)
{
    check(input, false);
}

identifier_list
ids()
{
    return identifier_list();
}

identifier_list
ids(std::string const& s1)
{
    identifier_list v;
    v.push_back(s1);
    return v;
}

identifier_list
ids(std::string const& s1, std::string const& s2)
{
    identifier_list v;
    v.push_back(s1);
    v.push_back(s2);
    return v;
}

identifier_list
ids(std::string const& s1, std::string const& s2, std::string const& s3)
{
    identifier_list v;
    v.push_back(s1);
    v.push_back(s2);
    v.push_back(s3);
    return v;
}

// Checks the decomposition of the input into appropriate values
void
checkValues(
    std::string const& input,
    int majorVersion,
    int minorVersion,
    int patchVersion,
    identifier_list const& preReleaseIdentifiers = identifier_list(),
    identifier_list const& metaData = identifier_list())
{
    SemanticVersion v;

    CHECK_UNARY(v.parse(input));

    CHECK_EQ(v.majorVersion, majorVersion);
    CHECK_EQ(v.minorVersion, minorVersion);
    CHECK_EQ(v.patchVersion, patchVersion);

    CHECK_EQ(v.preReleaseIdentifiers, preReleaseIdentifiers);
    CHECK_EQ(v.metaData, metaData);
}

// makes sure the left version is less than the right
void
checkLessInternal(std::string const& lhs, std::string const& rhs)
{
    SemanticVersion left;
    SemanticVersion right;

    CHECK_UNARY(left.parse(lhs));
    CHECK_UNARY(right.parse(rhs));

    CHECK_EQ(compare(left, left), 0);
    CHECK_EQ(compare(right, right), 0);
    CHECK_LT(compare(left, right), 0);
    CHECK_GT(compare(right, left), 0);

    CHECK_LT(left, right);
    CHECK_GT(right, left);
    CHECK_EQ(left, left);
    CHECK_EQ(right, right);
}

void
checkLess(std::string const& lhs, std::string const& rhs)
{
    checkLessInternal(lhs, rhs);
    checkLessInternal(lhs + "+meta", rhs);
    checkLessInternal(lhs, rhs + "+meta");
    checkLessInternal(lhs + "+meta", rhs + "+meta");
}

}  // namespace

TEST_CASE("parsing")
{
    check("0.0.0");
    check("1.2.3");
    check("2147483647.2147483647.2147483647");  // max int

    // negative values
    negcheck("-1.2.3");
    negcheck("1.-2.3");
    negcheck("1.2.-3");

    // missing parts
    negcheck("");
    negcheck("1");
    negcheck("1.");
    negcheck("1.2");
    negcheck("1.2.");
    negcheck(".2.3");

    // whitespace
    negcheck(" 1.2.3");
    negcheck("1 .2.3");
    negcheck("1.2 .3");
    negcheck("1.2.3 ");

    // leading zeroes
    negcheck("01.2.3");
    negcheck("1.02.3");
    negcheck("1.2.03");
}

TEST_CASE("values")
{
    checkValues("0.1.2", 0, 1, 2);
    checkValues("1.2.3", 1, 2, 3);
    checkValues("1.2.3-rc1", 1, 2, 3, ids("rc1"));
    checkValues("1.2.3-rc1.debug", 1, 2, 3, ids("rc1", "debug"));
    checkValues("1.2.3-rc1.debug.asm", 1, 2, 3, ids("rc1", "debug", "asm"));
    checkValues("1.2.3+full", 1, 2, 3, ids(), ids("full"));
    checkValues("1.2.3+full.prod", 1, 2, 3, ids(), ids("full", "prod"));
    checkValues(
        "1.2.3+full.prod.x86", 1, 2, 3, ids(), ids("full", "prod", "x86"));
    checkValues(
        "1.2.3-rc1.debug.asm+full.prod.x86",
        1,
        2,
        3,
        ids("rc1", "debug", "asm"),
        ids("full", "prod", "x86"));
}

TEST_CASE("comparisons")
{
    checkLess("1.0.0-alpha", "1.0.0-alpha.1");
    checkLess("1.0.0-alpha.1", "1.0.0-alpha.beta");
    checkLess("1.0.0-alpha.beta", "1.0.0-beta");
    checkLess("1.0.0-beta", "1.0.0-beta.2");
    checkLess("1.0.0-beta.2", "1.0.0-beta.11");
    checkLess("1.0.0-beta.11", "1.0.0-rc.1");
    checkLess("1.0.0-rc.1", "1.0.0");
    checkLess("0.9.9", "1.0.0");
}

TEST_SUITE_END();
