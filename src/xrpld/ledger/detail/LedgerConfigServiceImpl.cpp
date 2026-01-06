#include <xrpld/core/Config.h>
#include <xrpld/ledger/LedgerConfigServiceImpl.h>

namespace xrpl {

LedgerConfigServiceImpl::LedgerConfigServiceImpl(
    Config const& config,
    FeatureSetService& featureSetService)
    : featureSetService_(featureSetService)
    , referenceFee_(config.FEES.reference_fee)
    , accountReserve_(config.FEES.account_reserve)
    , ownerReserve_(config.FEES.owner_reserve)
{
}

LedgerConfig
LedgerConfigServiceImpl::getLedgerConfig() const
{
    return LedgerConfig{referenceFee_, accountReserve_, ownerReserve_};
}

FeatureSetService&
LedgerConfigServiceImpl::getFeatureSetService()
{
    return featureSetService_;
}

}  // namespace xrpl
