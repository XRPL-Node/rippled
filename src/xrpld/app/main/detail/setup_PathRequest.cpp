#include <xrpld/app/main/setup_PathRequest.h>
#include <xrpld/core/Config.h>

namespace xrpl {

PathRequest::Setup
setup_PathRequest(Config const& config)
{
    PathRequest::Setup setup;
    setup.pathSearch = config.PATH_SEARCH;
    setup.pathSearchFast = config.PATH_SEARCH_FAST;
    setup.pathSearchMax = config.PATH_SEARCH_MAX;
    return setup;
}

}  // namespace xrpl
