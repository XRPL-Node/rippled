#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/Transaction.h>
#include <xrpld/app/misc/detail/AccountTxPaging.h>

#include <xrpl/protocol/Serializer.h>

namespace xrpl {

void
convertBlobsToTxResult(
    RelationalDatabase::AccountTxs& to,
    std::uint32_t ledger_index,
    std::string const& status,
    Blob const& rawTxn,
    Blob const& rawMeta,
    ServiceRegistry& registry)
{
    SerialIter it(makeSlice(rawTxn));
    auto txn = std::make_shared<STTx const>(it);
    std::string reason;

    auto tr = std::make_shared<Transaction>(txn, reason, registry);

    auto metaset = std::make_shared<TxMeta>(tr->getID(), ledger_index, rawMeta);

    // if properly formed meta is available we can use it to generate ctid
    if (metaset->getAsObject().isFieldPresent(sfTransactionIndex))
        tr->setStatus(
            Transaction::sqlTransactionStatus(status),
            ledger_index,
            metaset->getAsObject().getFieldU32(sfTransactionIndex),
            registry.app().config().NETWORK_ID);
    else
        tr->setStatus(Transaction::sqlTransactionStatus(status), ledger_index);

    to.emplace_back(std::move(tr), metaset);
};

void
saveLedgerAsync(ServiceRegistry& registry, std::uint32_t seq)
{
    if (auto l = registry.getLedgerMaster().getLedgerBySeq(seq))
        pendSaveValidated(registry, l, false, false);
}

}  // namespace xrpl
