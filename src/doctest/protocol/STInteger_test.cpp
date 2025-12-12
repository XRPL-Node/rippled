#include <xrpl/protocol/LedgerFormats.h>
#include <xrpl/protocol/Permissions.h>
#include <xrpl/protocol/STInteger.h>
#include <xrpl/protocol/TxFormats.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("protocol");

TEST_CASE("STInteger_test - UInt8")
{
    STUInt8 u8(255);
    CHECK(u8.value() == 255);
    CHECK(u8.getText() == "255");
    CHECK(u8.getSType() == STI_UINT8);
    CHECK(u8.getJson(JsonOptions::none) == 255);

    // there is some special handling for sfTransactionResult
    STUInt8 tr(sfTransactionResult, 0);
    CHECK(tr.value() == 0);
    CHECK(
        tr.getText() ==
        "The transaction was applied. Only final in a validated ledger.");
    CHECK(tr.getSType() == STI_UINT8);
    CHECK(tr.getJson(JsonOptions::none) == "tesSUCCESS");

    // invalid transaction result
    STUInt8 tr2(sfTransactionResult, 255);
    CHECK(tr2.value() == 255);
    CHECK(tr2.getText() == "255");
    CHECK(tr2.getSType() == STI_UINT8);
    CHECK(tr2.getJson(JsonOptions::none) == 255);
}

TEST_CASE("STInteger_test - UInt16")
{
    STUInt16 u16(65535);
    CHECK(u16.value() == 65535);
    CHECK(u16.getText() == "65535");
    CHECK(u16.getSType() == STI_UINT16);
    CHECK(u16.getJson(JsonOptions::none) == 65535);

    // there is some special handling for sfLedgerEntryType
    STUInt16 let(sfLedgerEntryType, ltACCOUNT_ROOT);
    CHECK(let.value() == ltACCOUNT_ROOT);
    CHECK(let.getText() == "AccountRoot");
    CHECK(let.getSType() == STI_UINT16);
    CHECK(let.getJson(JsonOptions::none) == "AccountRoot");

    // there is some special handling for sfTransactionType
    STUInt16 tlt(sfTransactionType, ttPAYMENT);
    CHECK(tlt.value() == ttPAYMENT);
    CHECK(tlt.getText() == "Payment");
    CHECK(tlt.getSType() == STI_UINT16);
    CHECK(tlt.getJson(JsonOptions::none) == "Payment");
}

TEST_CASE("STInteger_test - UInt32")
{
    STUInt32 u32(4'294'967'295u);
    CHECK(u32.value() == 4'294'967'295u);
    CHECK(u32.getText() == "4294967295");
    CHECK(u32.getSType() == STI_UINT32);
    CHECK(u32.getJson(JsonOptions::none) == 4'294'967'295u);

    // there is some special handling for sfPermissionValue
    STUInt32 pv(sfPermissionValue, ttPAYMENT + 1);
    CHECK(pv.value() == ttPAYMENT + 1);
    CHECK(pv.getText() == "Payment");
    CHECK(pv.getSType() == STI_UINT32);
    CHECK(pv.getJson(JsonOptions::none) == "Payment");
    STUInt32 pv2(sfPermissionValue, PaymentMint);
    CHECK(pv2.value() == PaymentMint);
    CHECK(pv2.getText() == "PaymentMint");
    CHECK(pv2.getSType() == STI_UINT32);
    CHECK(pv2.getJson(JsonOptions::none) == "PaymentMint");
}

TEST_CASE("STInteger_test - UInt64")
{
    STUInt64 u64(0xFFFFFFFFFFFFFFFFull);
    CHECK(u64.value() == 0xFFFFFFFFFFFFFFFFull);
    CHECK(u64.getText() == "18446744073709551615");
    CHECK(u64.getSType() == STI_UINT64);

    // By default, getJson returns hex string
    auto jsonVal = u64.getJson(JsonOptions::none);
    CHECK(jsonVal.isString());
    CHECK(jsonVal.asString() == "ffffffffffffffff");

    STUInt64 u64_2(sfMaximumAmount, 0xFFFFFFFFFFFFFFFFull);
    CHECK(u64_2.value() == 0xFFFFFFFFFFFFFFFFull);
    CHECK(u64_2.getText() == "18446744073709551615");
    CHECK(u64_2.getSType() == STI_UINT64);
    CHECK(u64_2.getJson(JsonOptions::none) == "18446744073709551615");
}

TEST_CASE("STInteger_test - Int32")
{
    {
        int const minInt32 = -2147483648;
        STInt32 i32(minInt32);
        CHECK(i32.value() == minInt32);
        CHECK(i32.getText() == "-2147483648");
        CHECK(i32.getSType() == STI_INT32);
        CHECK(i32.getJson(JsonOptions::none) == minInt32);
    }

    {
        int const maxInt32 = 2147483647;
        STInt32 i32(maxInt32);
        CHECK(i32.value() == maxInt32);
        CHECK(i32.getText() == "2147483647");
        CHECK(i32.getSType() == STI_INT32);
        CHECK(i32.getJson(JsonOptions::none) == maxInt32);
    }
}

TEST_SUITE_END();
