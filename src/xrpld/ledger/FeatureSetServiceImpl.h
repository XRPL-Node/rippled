#ifndef XRPLD_LEDGER_FEATURESETSERVICEIMPL_H_INCLUDED
#define XRPLD_LEDGER_FEATURESETSERVICEIMPL_H_INCLUDED

#include <xrpl/core/FeatureSetService.h>

#include <unordered_set>

namespace xrpl {

// Forward declaration
class Config;

/** Implementation of FeatureSetService that reads from Config.

    This class provides a FeatureSetService interface that wraps
    the features set from the application Config. It caches the
    features at construction time.
*/
class FeatureSetServiceImpl : public FeatureSetService
{
public:
    explicit FeatureSetServiceImpl(Config const& config);

    ~FeatureSetServiceImpl() override = default;

    std::unordered_set<uint256, beast::uhash<>> const&
    features() const override;

private:
    Config const& config_;
};

}  // namespace xrpl

#endif
