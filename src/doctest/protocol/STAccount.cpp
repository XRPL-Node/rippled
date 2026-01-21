#include <xrpl/protocol/STAccount.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("STAccount");

TEST_CASE("STAccount default constructor")
{
    STAccount const defaultAcct;
    CHECK_EQ(defaultAcct.getSType(), STI_ACCOUNT);
    CHECK_EQ(defaultAcct.getText(), "");
    CHECK_UNARY(defaultAcct.isDefault());
    CHECK_EQ(defaultAcct.value(), AccountID{});
}

TEST_CASE("STAccount deserialized default")
{
    STAccount const defaultAcct;
    // Construct a deserialized default STAccount.
    Serializer s;
    s.addVL(nullptr, 0);
    SerialIter sit(s.slice());
    STAccount const deserializedDefault(sit, sfAccount);
    CHECK_UNARY(deserializedDefault.isEquivalent(defaultAcct));
}

TEST_CASE("STAccount constructor from SField")
{
    STAccount const defaultAcct;
    STAccount const sfAcct{sfAccount};
    CHECK_EQ(sfAcct.getSType(), STI_ACCOUNT);
    CHECK_EQ(sfAcct.getText(), "");
    CHECK_UNARY(sfAcct.isDefault());
    CHECK_EQ(sfAcct.value(), AccountID{});
    CHECK_UNARY(sfAcct.isEquivalent(defaultAcct));

    Serializer s;
    sfAcct.add(s);
    CHECK_EQ(s.size(), 1);
    CHECK_EQ(strHex(s), "00");
    SerialIter sit(s.slice());
    STAccount const deserializedSf(sit, sfAccount);
    CHECK_UNARY(deserializedSf.isEquivalent(sfAcct));
}

TEST_CASE("STAccount constructor from SField and AccountID")
{
    STAccount const defaultAcct;
    STAccount const sfAcct{sfAccount};
    STAccount const zeroAcct{sfAccount, AccountID{}};
    CHECK_EQ(zeroAcct.getText(), "rrrrrrrrrrrrrrrrrrrrrhoLvTp");
    CHECK_FALSE(zeroAcct.isDefault());
    CHECK_EQ(zeroAcct.value(), AccountID{0});
    CHECK_FALSE(zeroAcct.isEquivalent(defaultAcct));
    CHECK_FALSE(zeroAcct.isEquivalent(sfAcct));

    Serializer s;
    zeroAcct.add(s);
    CHECK_EQ(s.size(), 21);
    CHECK_EQ(strHex(s), "140000000000000000000000000000000000000000");
    SerialIter sit(s.slice());
    STAccount const deserializedZero(sit, sfAccount);
    CHECK_UNARY(deserializedZero.isEquivalent(zeroAcct));
}

TEST_CASE("STAccount bad size throws")
{
    // Construct from a VL that is not exactly 160 bits.
    Serializer s;
    std::uint8_t const bits128[]{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    s.addVL(bits128, sizeof(bits128));
    SerialIter sit(s.slice());
    CHECK_THROWS_AS(STAccount(sit, sfAccount), std::runtime_error);
}

TEST_CASE("STAccount equivalent types")
{
    STAccount const zeroAcct{sfAccount, AccountID{}};
    // Interestingly, equal values but different types are equivalent!
    STAccount const regKey{sfRegularKey, AccountID{}};
    CHECK_UNARY(regKey.isEquivalent(zeroAcct));
}

TEST_CASE("STAccount assignment")
{
    STAccount const defaultAcct;
    STAccount const zeroAcct{sfAccount, AccountID{}};

    STAccount assignAcct;
    CHECK_UNARY(assignAcct.isEquivalent(defaultAcct));
    CHECK_UNARY(assignAcct.isDefault());
    assignAcct = AccountID{};
    CHECK_FALSE(assignAcct.isEquivalent(defaultAcct));
    CHECK_UNARY(assignAcct.isEquivalent(zeroAcct));
    CHECK_FALSE(assignAcct.isDefault());
}

TEST_CASE("AccountID parsing")
{
    auto const s = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";
    auto const parsed = parseBase58<AccountID>(s);
    REQUIRE(parsed);
    CHECK_EQ(toBase58(*parsed), s);
}

TEST_CASE("AccountID invalid parsing")
{
    auto const s =
        "âabcd1rNxp4h8apvRis6mJf9Sh8C6iRxfrDWNâabcdAVâ\xc2\x80\xc2\x8f";
    CHECK_FALSE(parseBase58<AccountID>(s));
}

TEST_SUITE_END();
