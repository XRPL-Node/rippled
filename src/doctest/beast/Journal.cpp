#include <xrpl/beast/utility/Journal.h>

#include <doctest/doctest.h>

using namespace beast;

TEST_SUITE_BEGIN("Journal");

namespace {

class TestSink : public Journal::Sink
{
private:
    int m_count;

public:
    TestSink() : Sink(severities::kWarning, false), m_count(0)
    {
    }

    int
    count() const
    {
        return m_count;
    }

    void
    reset()
    {
        m_count = 0;
    }

    void
    write(severities::Severity level, std::string const&) override
    {
        if (level >= threshold())
            ++m_count;
    }

    void
    writeAlways(severities::Severity level, std::string const&) override
    {
        ++m_count;
    }
};

}  // namespace

TEST_CASE("Journal threshold kInfo")
{
    TestSink sink;

    using namespace beast::severities;
    sink.threshold(kInfo);

    Journal j(sink);

    j.trace() << " ";
    CHECK(sink.count() == 0);
    j.debug() << " ";
    CHECK(sink.count() == 0);
    j.info() << " ";
    CHECK(sink.count() == 1);
    j.warn() << " ";
    CHECK(sink.count() == 2);
    j.error() << " ";
    CHECK(sink.count() == 3);
    j.fatal() << " ";
    CHECK(sink.count() == 4);
}

TEST_CASE("Journal threshold kDebug")
{
    TestSink sink;

    using namespace beast::severities;
    sink.threshold(kDebug);

    Journal j(sink);

    j.trace() << " ";
    CHECK(sink.count() == 0);
    j.debug() << " ";
    CHECK(sink.count() == 1);
    j.info() << " ";
    CHECK(sink.count() == 2);
    j.warn() << " ";
    CHECK(sink.count() == 3);
    j.error() << " ";
    CHECK(sink.count() == 4);
    j.fatal() << " ";
    CHECK(sink.count() == 5);
}

TEST_SUITE_END();

