#ifndef XRPL_LEDGER_LEDGERCONFIG_H_INCLUDED
#define XRPL_LEDGER_LEDGERCONFIG_H_INCLUDED

#include <xrpl/protocol/XRPAmount.h>

namespace xrpl {

/** Configuration for creating Ledger objects.

    This struct contains the minimal configuration needed to construct
    a Ledger object.
*/
struct LedgerConfig
{
    /** Reference transaction fee */
    XRPAmount reference_fee;

    /** Base account reserve */
    XRPAmount account_reserve;

    /** Incremental owner reserve */
    XRPAmount owner_reserve;
};

}  // namespace xrpl

#endif
