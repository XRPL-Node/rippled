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
    CHECK(rv);
    CHECK(makeSlice(*rv) == makeSlice(strExpected));
}

void
testUnHexFailure(std::string const& strIn)
{
    auto rv = strUnHex(strIn);
    CHECK(!rv);
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
        CHECK(parseUrl(pUrl, "scheme://"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain.empty());
        CHECK(!pUrl.port);
        CHECK(pUrl.path.empty());
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme:///"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain.empty());
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "lower://domain"));
        CHECK(pUrl.scheme == "lower");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "domain");
        CHECK(!pUrl.port);
        CHECK(pUrl.path.empty());
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "UPPER://domain:234/"));
        CHECK(pUrl.scheme == "upper");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "domain");
        CHECK(*pUrl.port == 234);
        CHECK(pUrl.path == "/");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "Mixed://domain/path"));
        CHECK(pUrl.scheme == "mixed");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "domain");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/path");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://[::1]:123/path"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "::1");
        CHECK(*pUrl.port == 123);
        CHECK(pUrl.path == "/path");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://user:pass@domain:123/abc:321"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username == "user");
        CHECK(pUrl.password == "pass");
        CHECK(pUrl.domain == "domain");
        CHECK(*pUrl.port == 123);
        CHECK(pUrl.path == "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://user@domain:123/abc:321"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username == "user");
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "domain");
        CHECK(*pUrl.port == 123);
        CHECK(pUrl.path == "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://:pass@domain:123/abc:321"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password == "pass");
        CHECK(pUrl.domain == "domain");
        CHECK(*pUrl.port == 123);
        CHECK(pUrl.path == "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://domain:123/abc:321"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "domain");
        CHECK(*pUrl.port == 123);
        CHECK(pUrl.path == "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://user:pass@domain/abc:321"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username == "user");
        CHECK(pUrl.password == "pass");
        CHECK(pUrl.domain == "domain");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://user@domain/abc:321"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username == "user");
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "domain");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://:pass@domain/abc:321"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password == "pass");
        CHECK(pUrl.domain == "domain");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://domain/abc:321"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "domain");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/abc:321");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme:///path/to/file"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain.empty());
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/path/to/file");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://user:pass@domain/path/with/an@sign"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username == "user");
        CHECK(pUrl.password == "pass");
        CHECK(pUrl.domain == "domain");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/path/with/an@sign");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://domain/path/with/an@sign"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "domain");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/path/with/an@sign");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "scheme://:999/"));
        CHECK(pUrl.scheme == "scheme");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == ":999");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/");
    }

    {
        parsedURL pUrl;
        CHECK(parseUrl(pUrl, "http://::1:1234/validators"));
        CHECK(pUrl.scheme == "http");
        CHECK(pUrl.username.empty());
        CHECK(pUrl.password.empty());
        CHECK(pUrl.domain == "::0.1.18.52");
        CHECK(!pUrl.port);
        CHECK(pUrl.path == "/validators");
    }

    // Expected fails.
    {
        parsedURL pUrl;
        CHECK(!parseUrl(pUrl, ""));
        CHECK(!parseUrl(pUrl, "nonsense"));
        CHECK(!parseUrl(pUrl, "://"));
        CHECK(!parseUrl(pUrl, ":///"));
        CHECK(!parseUrl(pUrl, "scheme://user:pass@domain:65536/abc:321"));
        CHECK(!parseUrl(pUrl, "UPPER://domain:23498765/"));
        CHECK(!parseUrl(pUrl, "UPPER://domain:0/"));
        CHECK(!parseUrl(pUrl, "UPPER://domain:+7/"));
        CHECK(!parseUrl(pUrl, "UPPER://domain:-7234/"));
        CHECK(!parseUrl(pUrl, "UPPER://domain:@#$56!/"));
    }

    {
        std::string strUrl("s://" + std::string(8192, ':'));
        parsedURL pUrl;
        CHECK(!parseUrl(pUrl, strUrl));
    }
}

TEST_CASE("toString")
{
    auto result = to_string("hello");
    CHECK(result == "hello");
}

TEST_SUITE_END();
