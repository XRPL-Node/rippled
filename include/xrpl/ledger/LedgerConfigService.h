#ifndef XRPL_LEDGER_LEDGERCONFIGSERVICE_H_INCLUDED
#define XRPL_LEDGER_LEDGERCONFIGSERVICE_H_INCLUDED

#include <xrpl/ledger/LedgerConfig.h>

namespace xrpl {

// Forward declaration
class FeatureSetService;

/** Service that provides ledger configuration.

    This service creates LedgerConfig objects that contain the configuration
    values needed to create and manage ledgers.

    This is a service interface that can be accessed through ServiceRegistry,
    allowing components to get ledger configuration without depending on
    the full application Config class.
*/
class LedgerConfigService
{
public:
    virtual ~LedgerConfigService() = default;

    /** Get a LedgerConfig object with current configuration */
    virtual LedgerConfig
    getLedgerConfig() const = 0;

    /** Get the FeatureSetService */
    virtual FeatureSetService&
    getFeatureSetService() = 0;
};

}  // namespace xrpl

#endif
