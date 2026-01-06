#ifndef XRPL_MOCKLEDGERCONFIGSERVICE_H
#define XRPL_MOCKLEDGERCONFIGSERVICE_H

#include <test/core/MockFeatureSetService.h>

#include <xrpld/core/Config.h>

#include <xrpl/ledger/LedgerConfigService.h>

namespace xrpl::test {

class MockLedgerConfigService : public LedgerConfigService
{
public:
    MockLedgerConfigService(Config const& config)
        : config_(config), featureSetService_(config)
    {
    }

    LedgerConfig
    getLedgerConfig() const override
    {
        return LedgerConfig{
            config_.FEES.reference_fee,
            config_.FEES.account_reserve,
            config_.FEES.owner_reserve};
    }

    FeatureSetService&
    getFeatureSetService() override
    {
        return featureSetService_;
    }

private:
    Config const& config_;
    MockFeatureSetService featureSetService_;
};
}  // namespace xrpl::test

#endif  // XRPL_MOCKLEDGERCONFIGSERVICE_H
