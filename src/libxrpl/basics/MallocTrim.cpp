#include <xrpl/basics/Log.h>
#include <xrpl/basics/MallocTrim.h>

#include <boost/predef.h>

#include <cstdio>
#include <fstream>

#if defined(__GLIBC__) && BOOST_OS_LINUX
#include <malloc.h>
#include <unistd.h>

namespace {
pid_t const cachedPid = ::getpid();
}  // namespace
#endif

namespace ripple {

namespace detail {

#if defined(__GLIBC__) && BOOST_OS_LINUX

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
#else

    report.supported = true;

    if (journal.debug())
    {
        auto readFile = [](std::string const& path) -> std::string {
            std::ifstream ifs(path);
            if (!ifs.is_open())
                return {};
            return std::string(
                std::istreambuf_iterator<char>(ifs),
                std::istreambuf_iterator<char>());
        };

        std::string const tagStr = tag.value_or("default");
        std::string const statusPath =
            "/proc/" + std::to_string(cachedPid) + "/status";

        auto const statusBefore = readFile(statusPath);
        report.rssBeforeKB = detail::parseVmRSSkB(statusBefore);

        report.trimResult = ::malloc_trim(0);

        auto const statusAfter = readFile(statusPath);
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
#endif

    return report;
}

}  // namespace ripple
