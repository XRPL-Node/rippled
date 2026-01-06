#ifndef XRPL_LEDGER_FEATURESETSERVICE_H_INCLUDED
#define XRPL_LEDGER_FEATURESETSERVICE_H_INCLUDED

#include <xrpl/beast/hash/uhash.h>

#include <unordered_set>

namespace xrpl {

/** Service that provides access to enabled features/amendments.

    This service provides read-only access to the set of enabled features
    (amendments) that are active in the ledger. Components can use this
    service to check which features are enabled without depending on the
    full application Config class.
*/
class FeatureSetService
{
public:
    virtual ~FeatureSetService() = default;

    /** Get the set of enabled features/amendments */
    virtual std::unordered_set<uint256, beast::uhash<>> const&
    features() const = 0;
};

}  // namespace xrpl

#endif
