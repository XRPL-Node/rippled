#ifndef XRPLD_LEDGER_LEDGERCONFIGSERVICEIMPL_H_INCLUDED
#define XRPLD_LEDGER_LEDGERCONFIGSERVICEIMPL_H_INCLUDED

#include <xrpl/ledger/LedgerConfigService.h>
#include <xrpl/protocol/XRPAmount.h>

namespace xrpl {

// Forward declarations
class Config;
class FeatureSetService;

/** Implementation of LedgerConfigService that reads from Config.

    This class provides a LedgerConfigService interface that creates
    LedgerConfig objects from the application Config. It caches the
    fee values at construction time and holds a reference to the
    FeatureSetService.
*/
class LedgerConfigServiceImpl : public LedgerConfigService
{
public:
    LedgerConfigServiceImpl(
        Config const& config,
        FeatureSetService& featureSetService);

    ~LedgerConfigServiceImpl() override = default;

    LedgerConfig
    getLedgerConfig() const override;

    FeatureSetService&
    getFeatureSetService() override;

private:
    FeatureSetService& featureSetService_;
    XRPAmount referenceFee_;
    XRPAmount accountReserve_;
    XRPAmount ownerReserve_;
};

}  // namespace xrpl

#endif
