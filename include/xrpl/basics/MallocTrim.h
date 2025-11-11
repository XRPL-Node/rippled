#ifndef XRPL_BASICS_MALLOCTRIM_H_INCLUDED
#define XRPL_BASICS_MALLOCTRIM_H_INCLUDED

#include <xrpl/beast/utility/Journal.h>

#include <optional>
#include <string>

namespace ripple {

struct MallocTrimReport
{
    bool supported{false};
    int trimResult{-1};
    long rssBeforeKB{-1};
    long rssAfterKB{-1};

    long
    deltaKB() const
    {
        return rssAfterKB - rssBeforeKB;
    }
};

/** Attempt to return freed memory to the operating system.
 *
 * This function is only effective on Linux with glibc. On other platforms,
 * it returns a report indicating the operation is unsupported.
 *
 * @param tag Optional identifier for logging/debugging purposes
 * @param journal Journal for logging diagnostic information
 * @return Report containing before/after memory metrics
 *
 * @note This is intended for use after cache sweeps or other operations
 *       that free significant amounts of memory.
 */
MallocTrimReport
mallocTrim(std::optional<std::string> const& tag, beast::Journal journal);

}  // namespace ripple

#endif
