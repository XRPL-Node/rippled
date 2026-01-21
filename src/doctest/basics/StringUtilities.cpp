#include <xrpl/basics/Slice.h>
#include <xrpl/basics/StringUtilities.h>
#include <xrpl/basics/ToString.h>

#include <doctest/doctest.h>

using namespace xrpl;

namespace {
void
testUnHexSuccess(std::string const& strIn, std::string const& strExpected)
{
    auto rv = strUnHex(strIn);
    CHECK_UNARY(rv);
    CHECK_EQ(makeSlice(*rv), makeSlice(strExpected));
}

void
testUnHexFailure(std::string const& strIn)
{
    auto rv = strUnHex(strIn);
    CHECK_FALSE(rv);
}
}  // namespace

TEST_SUITE_BEGIN("StringUtilities");

TEST_CASE("strUnHex")
{
    testUnHexSuccess("526970706c6544", "RippleD");
    testUnHexSuccess("A", "\n");
    testUnHexSuccess("0A", "\n");
    testUnHexSuccess("D0A", "\r\n");
    testUnHexSuccess("0D0A", "\r\n");
    testUnHexSuccess("200D0A", " \r\n");
    testUnHexSuccess("282A2B2C2D2E2F29", "(*+,-./)");

    // Check for things which contain some or only invalid characters
    testUnHexFailure("123X");
    testUnHexFailure("V");
    testUnHexFailure("XRP");
}

TEST_CASE("parseUrl")
{
    // Expected passes.
    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_UNARY(pUrl.domain.empty());
        CHECK_FALSE(pUrl.port);
        CHECK_UNARY(pUrl.path.empty());
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme:///"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_UNARY(pUrl.domain.empty());
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "lower://domain"));
        CHECK_EQ(pUrl.scheme, "lower");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_FALSE(pUrl.port);
        CHECK_UNARY(pUrl.path.empty());
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "UPPER://domain:234/"));
        CHECK_EQ(pUrl.scheme, "upper");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_EQ(*pUrl.port, 234);
        CHECK_EQ(pUrl.path, "/");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "Mixed://domain/path"));
        CHECK_EQ(pUrl.scheme, "mixed");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/path");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://[::1]:123/path"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "::1");
        CHECK_EQ(*pUrl.port, 123);
        CHECK_EQ(pUrl.path, "/path");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://user:pass@domain:123/abc:321"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_EQ(pUrl.username, "user");
        CHECK_EQ(pUrl.password, "pass");
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_EQ(*pUrl.port, 123);
        CHECK_EQ(pUrl.path, "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://user@domain:123/abc:321"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_EQ(pUrl.username, "user");
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_EQ(*pUrl.port, 123);
        CHECK_EQ(pUrl.path, "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://:pass@domain:123/abc:321"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_EQ(pUrl.password, "pass");
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_EQ(*pUrl.port, 123);
        CHECK_EQ(pUrl.path, "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://domain:123/abc:321"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_EQ(*pUrl.port, 123);
        CHECK_EQ(pUrl.path, "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://user:pass@domain/abc:321"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_EQ(pUrl.username, "user");
        CHECK_EQ(pUrl.password, "pass");
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://user@domain/abc:321"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_EQ(pUrl.username, "user");
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://:pass@domain/abc:321"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_EQ(pUrl.password, "pass");
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://domain/abc:321"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme:///path/to/file"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_UNARY(pUrl.domain.empty());
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/path/to/file");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(
            parseUrl(pUrl, "scheme://user:pass@domain/path/with/an@sign"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_EQ(pUrl.username, "user");
        CHECK_EQ(pUrl.password, "pass");
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/path/with/an@sign");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://domain/path/with/an@sign"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "domain");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/path/with/an@sign");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "scheme://:999/"));
        CHECK_EQ(pUrl.scheme, "scheme");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, ":999");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/");
    }

    {
        parsedURL pUrl;
        CHECK_UNARY(parseUrl(pUrl, "http://::1:1234/validators"));
        CHECK_EQ(pUrl.scheme, "http");
        CHECK_UNARY(pUrl.username.empty());
        CHECK_UNARY(pUrl.password.empty());
        CHECK_EQ(pUrl.domain, "::0.1.18.52");
        CHECK_FALSE(pUrl.port);
        CHECK_EQ(pUrl.path, "/validators");
    }

    // Expected fails.
    {
        parsedURL pUrl;
        CHECK_FALSE(parseUrl(pUrl, ""));
        CHECK_FALSE(parseUrl(pUrl, "nonsense"));
        CHECK_FALSE(parseUrl(pUrl, "://"));
        CHECK_FALSE(parseUrl(pUrl, ":///"));
        CHECK_FALSE(parseUrl(pUrl, "scheme://user:pass@domain:65536/abc:321"));
        CHECK_FALSE(parseUrl(pUrl, "UPPER://domain:23498765/"));
        CHECK_FALSE(parseUrl(pUrl, "UPPER://domain:0/"));
        CHECK_FALSE(parseUrl(pUrl, "UPPER://domain:+7/"));
        CHECK_FALSE(parseUrl(pUrl, "UPPER://domain:-7234/"));
        CHECK_FALSE(parseUrl(pUrl, "UPPER://domain:@#$56!/"));
    }

    {
        std::string strUrl("s://" + std::string(8192, ':'));
        parsedURL pUrl;
        CHECK_FALSE(parseUrl(pUrl, strUrl));
    }
}

TEST_CASE("toString")
{
    auto result = to_string("hello");
    CHECK_EQ(result, "hello");
}

TEST_SUITE_END();
