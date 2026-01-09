#ifndef XRPLD_APP_PATHS_SETUP_PATHREQUEST_H_INCLUDED
#define XRPLD_APP_PATHS_SETUP_PATHREQUEST_H_INCLUDED

#include <xrpld/app/paths/PathRequest.h>

namespace xrpl {

// Forward declaration
class Config;

/** Create PathRequest setup from configuration */
PathRequest::Setup
setup_PathRequest(Config const& config);

}  // namespace xrpl

#endif
