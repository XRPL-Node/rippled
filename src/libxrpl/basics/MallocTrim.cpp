#include <xrpl/basics/Log.h>
#include <xrpl/basics/MallocTrim.h>

#include <boost/predef.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>

#if defined(__GLIBC__) && BOOST_OS_LINUX
#include <malloc.h>
#include <unistd.h>
#endif

namespace ripple {

namespace {
#if defined(__GLIBC__) && BOOST_OS_LINUX
std::atomic<bool> isTrimming{false};
std::atomic<int64_t> lastTrimTimeMs{0};
constexpr int64_t minTrimIntervalMs = 5000;  // TODO: derive from somewhere
pid_t const cachedPid = ::getpid();
#endif
}  // namespace

namespace detail {

#if defined(__GLIBC__) && BOOST_OS_LINUX

std::string
readFile(std::string const& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
        return {};
    return std::string(
        std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

long
parseVmRSSkB(std::string const& status)
{
    // "VmRSS:      123456 kB"
    auto pos = status.find("VmRSS:");
    if (pos == std::string::npos)
        return -1;

    pos += 6;  // past "VmRSS:"
    while (pos < status.size() && status[pos] == ' ')
        ++pos;

    long value = -1;
    std::sscanf(status.c_str() + pos, "%ld", &value);
    return value;  // in kB
}

#endif  // __GLIBC__ && BOOST_OS_LINUX

}  // namespace detail

MallocTrimReport
mallocTrim(
    [[maybe_unused]] std::optional<std::string> const& tag,
    beast::Journal journal)
{
    MallocTrimReport report;

#if !(defined(__GLIBC__) && BOOST_OS_LINUX)
    JLOG(journal.debug()) << "malloc_trim not supported on this platform";
    return report;
#else

    report.supported = true;

    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now().time_since_epoch())
                     .count();

    if (nowMs - lastTrimTimeMs.load(std::memory_order_relaxed) <
        minTrimIntervalMs)
    {
        JLOG(journal.debug()) << "malloc_trim skipped - rate limited";
        return report;
    }

    bool expected = false;
    if (!isTrimming.compare_exchange_strong(
            expected, true, std::memory_order_acquire))
    {
        JLOG(journal.debug()) << "malloc_trim skipped - already in progress";
        return report;
    }

    nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();

    if (nowMs - lastTrimTimeMs.load(std::memory_order_relaxed) <
        minTrimIntervalMs)
    {
        isTrimming.store(false, std::memory_order_release);
        JLOG(journal.debug())
            << "malloc_trim skipped - rate limited (double check)";
        return report;
    }

    if (journal.debug())
    {
        std::string const tagStr = tag.value_or("default");
        std::string const statusPath =
            "/proc/" + std::to_string(cachedPid) + "/status";

        auto const statusBefore = detail::readFile(statusPath);
        report.rssBeforeKB = detail::parseVmRSSkB(statusBefore);

        report.trimResult = ::malloc_trim(0);

        auto const statusAfter = detail::readFile(statusPath);
        report.rssAfterKB = detail::parseVmRSSkB(statusAfter);

        JLOG(journal.debug())
            << "malloc_trim tag=" << tagStr << " result=" << report.trimResult
            << " rss_before=" << report.rssBeforeKB << "kB"
            << " rss_after=" << report.rssAfterKB << "kB"
            << " delta=" << report.deltaKB() << "kB";
    }
    else
    {
        report.trimResult = ::malloc_trim(0);
    }

    lastTrimTimeMs.store(nowMs, std::memory_order_relaxed);
    isTrimming.store(false, std::memory_order_release);

    return report;
#endif
}

}  // namespace ripple
