#pragma once

#include <xrpl/json/json_value.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/STObject.h>

namespace xrpl::ledger_entries {

/**
 * Base class for all ledger entry builders.
 * Provides common field setters that are available for all ledger entry types.
 */
template <typename Derived>
class LedgerEntryBuilderBase
{
public:
    /**
     * Set the ledger index.
     * @param value Ledger index
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setLedgerIndex(uint256 const& value)
    {
        object_[sfLedgerIndex] = value;
        return static_cast<Derived&>(*this);
    }

    /**
     * Set the flags.
     * @param value Flags value
     * @return Reference to the derived builder for method chaining.
     */
    Derived&
    setFlags(uint32_t value)
    {
        object_.setFieldU32(sfFlags, value);
        return static_cast<Derived&>(*this);
    }

    /**
     * @brief Factory method to create a new instance of the derived builder.
     *
     * Creates a default-constructed builder instance. It is recommended to use
     * this factory method instead of directly constructing the derived type to
     * avoid creating unnecessary temporary objects.
     * @return A new instance of the derived builder type
     */
    static Derived
    create()
    {
        return Derived{};
    }

    /**
     * @brief Factory method to create an instance of the derived builder from an existing SLE.
     *
     * Creates a builder instance initialized with data from an existing serialized
     * ledger entry. It is recommended to use this factory method instead of directly
     * constructing the derived type to avoid creating unnecessary temporary objects.
     * @param sle The existing serialized ledger entry to initialize from
     * @return A new instance of the derived builder type initialized with the SLE data
     */
    static Derived
    create(SLE const& sle)
    {
        return Derived{sle};
    }

protected:
    STObject object_{sfLedgerEntry};
};

}  // namespace xrpl::ledger_entries
