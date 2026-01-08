#ifndef XRPL_APP_RDB_BACKEND_SQLITEDATABASE_H_INCLUDED
#define XRPL_APP_RDB_BACKEND_SQLITEDATABASE_H_INCLUDED

#include <xrpl/rdb/RelationalDatabase.h>

#include <memory>

namespace xrpl {

class Config;
class JobQueue;
class ServiceRegistry;

class SQLiteDatabase final : public RelationalDatabase
{
public:
    SQLiteDatabase(
        ServiceRegistry& registry,
        Config const& config,
        JobQueue& jobQueue)
        : registry_(registry)
        , useTxTables_(config.useTxTables())
        , j_(registry.journal("SQLiteDatabase"))
    {
        DatabaseCon::Setup const setup = setup_DatabaseCon(config, j_);
        if (!makeLedgerDBs(
                config,
                setup,
                DatabaseCon::CheckpointerSetup{&jobQueue, &registry_.logs()}))
        {
            std::string_view constexpr error =
                "Failed to create ledger databases";

            JLOG(j_.fatal()) << error;
            Throw<std::runtime_error>(error.data());
        }
    }

    std::optional<LedgerIndex>
    getMinLedgerSeq() override;

    std::optional<LedgerIndex>
    getTransactionsMinLedgerSeq() override;

    std::optional<LedgerIndex>
    getAccountTransactionsMinLedgerSeq() override;

    std::optional<LedgerIndex>
    getMaxLedgerSeq() override;

    void
    deleteTransactionByLedgerSeq(LedgerIndex ledgerSeq) override;

    void
    deleteBeforeLedgerSeq(LedgerIndex ledgerSeq) override;

    void
    deleteTransactionsBeforeLedgerSeq(LedgerIndex ledgerSeq) override;

    void
    deleteAccountTransactionsBeforeLedgerSeq(LedgerIndex ledgerSeq) override;

    std::size_t
    getTransactionCount() override;

    std::size_t
    getAccountTransactionCount() override;

    RelationalDatabase::CountMinMax
    getLedgerCountMinMax() override;

    bool
    saveValidatedLedger(
        std::shared_ptr<Ledger const> const& ledger,
        bool current) override;

    std::optional<LedgerHeader>
    getLedgerInfoByIndex(LedgerIndex ledgerSeq) override;

    std::optional<LedgerHeader>
    getNewestLedgerInfo() override;

    std::optional<LedgerHeader>
    getLimitedOldestLedgerInfo(LedgerIndex ledgerFirstIndex) override;

    std::optional<LedgerHeader>
    getLimitedNewestLedgerInfo(LedgerIndex ledgerFirstIndex) override;

    std::optional<LedgerHeader>
    getLedgerInfoByHash(uint256 const& ledgerHash) override;

    uint256
    getHashByIndex(LedgerIndex ledgerIndex) override;

    std::optional<LedgerHashPair>
    getHashesByIndex(LedgerIndex ledgerIndex) override;

    std::map<LedgerIndex, LedgerHashPair>
    getHashesByIndex(LedgerIndex minSeq, LedgerIndex maxSeq) override;

    std::vector<std::shared_ptr<Transaction>>
    getTxHistory(LedgerIndex startIndex) override;

    AccountTxs
    getOldestAccountTxs(AccountTxOptions const& options) override;

    AccountTxs
    getNewestAccountTxs(AccountTxOptions const& options) override;

    MetaTxsList
    getOldestAccountTxsB(AccountTxOptions const& options) override;

    MetaTxsList
    getNewestAccountTxsB(AccountTxOptions const& options) override;

    std::pair<AccountTxs, std::optional<AccountTxMarker>>
    oldestAccountTxPage(AccountTxPageOptions const& options) override;

    std::pair<AccountTxs, std::optional<AccountTxMarker>>
    newestAccountTxPage(AccountTxPageOptions const& options) override;

    std::pair<MetaTxsList, std::optional<AccountTxMarker>>
    oldestAccountTxPageB(AccountTxPageOptions const& options) override;

    std::pair<MetaTxsList, std::optional<AccountTxMarker>>
    newestAccountTxPageB(AccountTxPageOptions const& options) override;

    std::variant<AccountTx, TxSearched>
    getTransaction(
        uint256 const& id,
        std::optional<ClosedInterval<std::uint32_t>> const& range,
        error_code_i& ec) override;

    std::uint32_t
    getKBUsedAll() override;

    std::uint32_t
    getKBUsedLedger() override;

    std::uint32_t
    getKBUsedTransaction() override;

    void
    closeLedgerDB() override;

    void
    closeTransactionDB() override;

    /**
     * @brief ledgerDbHasSpace Checks if the ledger database has available
     *        space.
     * @param config Config object.
     * @return True if space is available.
     */
    bool
    ledgerDbHasSpace(Config const& config);

    /**
     * @brief transactionDbHasSpace Checks if the transaction database has
     *        available space.
     * @param config Config object.
     * @return True if space is available.
     */
    bool
    transactionDbHasSpace(Config const& config);

private:
    ServiceRegistry& registry_;
    bool const useTxTables_;
    beast::Journal j_;
    std::unique_ptr<DatabaseCon> lgrdb_, txdb_;

    /**
     * @brief makeLedgerDBs Opens ledger and transaction databases for the node
     *        store, and stores their descriptors in private member variables.
     * @param config Config object.
     * @param setup Path to the databases and other opening parameters.
     * @param checkpointerSetup Checkpointer parameters.
     * @return True if node databases opened successfully.
     */
    bool
    makeLedgerDBs(
        Config const& config,
        DatabaseCon::Setup const& setup,
        DatabaseCon::CheckpointerSetup const& checkpointerSetup);

    /**
     * @brief existsLedger Checks if the node store ledger database exists.
     * @return True if the node store ledger database exists.
     */
    bool
    existsLedger()
    {
        return static_cast<bool>(lgrdb_);
    }

    /**
     * @brief existsTransaction Checks if the node store transaction database
     *        exists.
     * @return True if the node store transaction database exists.
     */
    bool
    existsTransaction()
    {
        return static_cast<bool>(txdb_);
    }

    /**
     * @brief checkoutTransaction Checks out and returns node store ledger
     *        database.
     * @return Session to the node store ledger database.
     */
    auto
    checkoutLedger()
    {
        return lgrdb_->checkoutDb();
    }

    /**
     * @brief checkoutTransaction Checks out and returns the node store
     *        transaction database.
     * @return Session to the node store transaction database.
     */
    auto
    checkoutTransaction()
    {
        return txdb_->checkoutDb();
    }
};

/**
 * @brief setup_RelationalDatabase Creates and returns a SQLiteDatabase
 *        instance based on configuration.
 * @param registry The service registry.
 * @param config Config object.
 * @param jobQueue JobQueue object.
 * @return Unique pointer to the SQLiteDatabase implementation.
 */
std::unique_ptr<SQLiteDatabase>
setup_RelationalDatabase(
    ServiceRegistry& registry,
    Config const& config,
    JobQueue& jobQueue);

}  // namespace xrpl

#endif
