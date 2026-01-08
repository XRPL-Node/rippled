#include <xrpld/app/main/Application.h>
#include <xrpld/rpc/Context.h>

#include <xrpl/json/json_value.h>
#include <xrpl/overlay/Overlay.h>

namespace xrpl {

Json::Value
doTxReduceRelay(RPC::JsonContext& context)
{
    return context.app.overlay().txMetrics();
}

}  // namespace xrpl
