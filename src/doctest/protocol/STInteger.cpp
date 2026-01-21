#include <xrpl/protocol/LedgerFormats.h>
#include <xrpl/protocol/Permissions.h>
#include <xrpl/protocol/STInteger.h>
#include <xrpl/protocol/TxFormats.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("STInteger");

TEST_CASE("UInt8")
{
    STUInt8 u8(255);
    CHECK_EQ(u8.value(), 255);
    CHECK_EQ(u8.getText(), "255");
    CHECK_EQ(u8.getSType(), STI_UINT8);
    CHECK_EQ(u8.getJson(JsonOptions::none), 255);

    // there is some special handling for sfTransactionResult
    STUInt8 tr(sfTransactionResult, 0);
    CHECK_EQ(tr.value(), 0);
    CHECK_EQ(
        tr.getText(),
        "The transaction was applied. Only final in a validated ledger.");
    CHECK_EQ(tr.getSType(), STI_UINT8);
    CHECK_EQ(tr.getJson(JsonOptions::none), "tesSUCCESS");

    // invalid transaction result
    STUInt8 tr2(sfTransactionResult, 255);
    CHECK_EQ(tr2.value(), 255);
    CHECK_EQ(tr2.getText(), "255");
    CHECK_EQ(tr2.getSType(), STI_UINT8);
    CHECK_EQ(tr2.getJson(JsonOptions::none), 255);
}

TEST_CASE("UInt16")
{
    STUInt16 u16(65535);
    CHECK_EQ(u16.value(), 65535);
    CHECK_EQ(u16.getText(), "65535");
    CHECK_EQ(u16.getSType(), STI_UINT16);
    CHECK_EQ(u16.getJson(JsonOptions::none), 65535);

    // there is some special handling for sfLedgerEntryType
    STUInt16 let(sfLedgerEntryType, ltACCOUNT_ROOT);
    CHECK_EQ(let.value(), ltACCOUNT_ROOT);
    CHECK_EQ(let.getText(), "AccountRoot");
    CHECK_EQ(let.getSType(), STI_UINT16);
    CHECK_EQ(let.getJson(JsonOptions::none), "AccountRoot");

    // there is some special handling for sfTransactionType
    STUInt16 tlt(sfTransactionType, ttPAYMENT);
    CHECK_EQ(tlt.value(), ttPAYMENT);
    CHECK_EQ(tlt.getText(), "Payment");
    CHECK_EQ(tlt.getSType(), STI_UINT16);
    CHECK_EQ(tlt.getJson(JsonOptions::none), "Payment");
}

TEST_CASE("UInt32")
{
    STUInt32 u32(4'294'967'295u);
    CHECK_EQ(u32.value(), 4'294'967'295u);
    CHECK_EQ(u32.getText(), "4294967295");
    CHECK_EQ(u32.getSType(), STI_UINT32);
    CHECK_EQ(u32.getJson(JsonOptions::none), 4'294'967'295u);

    // there is some special handling for sfPermissionValue
    STUInt32 pv(sfPermissionValue, ttPAYMENT + 1);
    CHECK_EQ(pv.value(), ttPAYMENT + 1);
    CHECK_EQ(pv.getText(), "Payment");
    CHECK_EQ(pv.getSType(), STI_UINT32);
    CHECK_EQ(pv.getJson(JsonOptions::none), "Payment");
    STUInt32 pv2(sfPermissionValue, PaymentMint);
    CHECK_EQ(pv2.value(), PaymentMint);
    CHECK_EQ(pv2.getText(), "PaymentMint");
    CHECK_EQ(pv2.getSType(), STI_UINT32);
    CHECK_EQ(pv2.getJson(JsonOptions::none), "PaymentMint");
}

TEST_CASE("UInt64")
{
    STUInt64 u64(0xFFFFFFFFFFFFFFFFull);
    CHECK_EQ(u64.value(), 0xFFFFFFFFFFFFFFFFull);
    CHECK_EQ(u64.getText(), "18446744073709551615");
    CHECK_EQ(u64.getSType(), STI_UINT64);

    // By default, getJson returns hex string
    auto jsonVal = u64.getJson(JsonOptions::none);
    CHECK_UNARY(jsonVal.isString());
    CHECK_EQ(jsonVal.asString(), "ffffffffffffffff");

    STUInt64 u64_2(sfMaximumAmount, 0xFFFFFFFFFFFFFFFFull);
    CHECK_EQ(u64_2.value(), 0xFFFFFFFFFFFFFFFFull);
    CHECK_EQ(u64_2.getText(), "18446744073709551615");
    CHECK_EQ(u64_2.getSType(), STI_UINT64);
    CHECK_EQ(u64_2.getJson(JsonOptions::none), "18446744073709551615");
}

TEST_CASE("Int32")
{
    SUBCASE("min value")
    {
        int const minInt32 = -2147483648;
        STInt32 i32(minInt32);
        CHECK_EQ(i32.value(), minInt32);
        CHECK_EQ(i32.getText(), "-2147483648");
        CHECK_EQ(i32.getSType(), STI_INT32);
        CHECK_EQ(i32.getJson(JsonOptions::none), minInt32);
    }

    SUBCASE("max value")
    {
        int const maxInt32 = 2147483647;
        STInt32 i32(maxInt32);
        CHECK_EQ(i32.value(), maxInt32);
        CHECK_EQ(i32.getText(), "2147483647");
        CHECK_EQ(i32.getSType(), STI_INT32);
        CHECK_EQ(i32.getJson(JsonOptions::none), maxInt32);
    }
}

TEST_SUITE_END();
