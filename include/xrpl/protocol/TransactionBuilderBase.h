#pragma once

#include <xrpl/json/json_value.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/jss.h>

namespace xrpl::transactions {

/**
 * Base class for all transaction builders.
 * Provides common field setters that are available for all transaction types.
 */
template <typename Derived>
class TransactionBuilderBase
{
public:
    /**
     * Set the account that is sending the transaction.
     * @param value Account address (typically as a string)
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setAccount(AccountID const& value)
    {
        set(object_, sfAccount, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * Set the transaction fee.
     * @param value Fee in drops (typically as a string or number)
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setFee(STAmount const& value)
    {
        set(object_, sfFee, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * Set the sequence number.
     * @param value Sequence number
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setSequence(std::uint32_t const& value)
    {
        set(object_, sfSequence, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * Set the signing public key.
     * @param value Public key (typically as a hex string)
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setSigningPubKey(Blob const& value)
    {
        set(object_, sfSigningPubKey, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * Set transaction flags.
     * @param value Flags value
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setFlags(std::uint32_t const& value)
    {
        set(object_, sfFlags, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * Set the source tag.
     * @param value Source tag
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setSourceTag(std::uint32_t const& value)
    {
        set(object_, sfSourceTag, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * Set the last ledger sequence.
     * @param value Last ledger sequence number
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setLastLedgerSequence(std::uint32_t const& value)
    {
        set(object_, sfLastLedgerSequence, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * Set the account transaction ID.
     * @param value Account transaction ID (typically as a hex string)
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setAccountTxnID(STUInt256 const& value)
    {
        set(object_, sfAccountTxnID, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * @brief Factory method to create an instance of the derived builder.
     *
     * @return A new instance of the derived builder type
     */
    static Derived
    create()
    {
        return Derived{};
    }

protected:
    STObject object_{sfTransaction};
};

}  // namespace xrpl::transactions
