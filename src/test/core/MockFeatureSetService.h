#ifndef XRPL_MOCKFEATURESETSERVICE_H
#define XRPL_MOCKFEATURESETSERVICE_H

#include <xrpld/core/Config.h>

#include <xrpl/core/FeatureSetService.h>

namespace xrpl::test {

class MockFeatureSetService : public FeatureSetService
{
public:
    MockFeatureSetService(Config const& config) : config_(config)
    {
    }

    std::unordered_set<uint256, beast::uhash<>> const&
    features() const override
    {
        return config_.features;
    }

private:
    Config const& config_;
};
}  // namespace xrpl::test

#endif  // XRPL_MOCKFEATURESETSERVICE_H
