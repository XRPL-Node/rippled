#include <xrpld/core/Config.h>
#include <xrpld/ledger/FeatureSetServiceImpl.h>

namespace xrpl {

FeatureSetServiceImpl::FeatureSetServiceImpl(Config const& config)
    : config_(config)
{
}

std::unordered_set<uint256, beast::uhash<>> const&
FeatureSetServiceImpl::features() const
{
    return config_.features;
}

}  // namespace xrpl
