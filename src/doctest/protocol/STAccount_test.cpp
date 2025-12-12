#include <xrpl/protocol/STAccount.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("protocol");

TEST_CASE("STAccount default constructor")
{
    STAccount const defaultAcct;
    CHECK(defaultAcct.getSType() == STI_ACCOUNT);
    CHECK(defaultAcct.getText() == "");
    CHECK(defaultAcct.isDefault() == true);
    CHECK(defaultAcct.value() == AccountID{});
}

TEST_CASE("STAccount deserialized default")
{
    STAccount const defaultAcct;
    // Construct a deserialized default STAccount.
    Serializer s;
    s.addVL(nullptr, 0);
    SerialIter sit(s.slice());
    STAccount const deserializedDefault(sit, sfAccount);
    CHECK(deserializedDefault.isEquivalent(defaultAcct));
}

TEST_CASE("STAccount constructor from SField")
{
    STAccount const defaultAcct;
    STAccount const sfAcct{sfAccount};
    CHECK(sfAcct.getSType() == STI_ACCOUNT);
    CHECK(sfAcct.getText() == "");
    CHECK(sfAcct.isDefault());
    CHECK(sfAcct.value() == AccountID{});
    CHECK(sfAcct.isEquivalent(defaultAcct));

    Serializer s;
    sfAcct.add(s);
    CHECK(s.size() == 1);
    CHECK(strHex(s) == "00");
    SerialIter sit(s.slice());
    STAccount const deserializedSf(sit, sfAccount);
    CHECK(deserializedSf.isEquivalent(sfAcct));
}

TEST_CASE("STAccount constructor from SField and AccountID")
{
    STAccount const defaultAcct;
    STAccount const sfAcct{sfAccount};
    STAccount const zeroAcct{sfAccount, AccountID{}};
    CHECK(zeroAcct.getText() == "rrrrrrrrrrrrrrrrrrrrrhoLvTp");
    CHECK(!zeroAcct.isDefault());
    CHECK(zeroAcct.value() == AccountID{0});
    CHECK(!zeroAcct.isEquivalent(defaultAcct));
    CHECK(!zeroAcct.isEquivalent(sfAcct));

    Serializer s;
    zeroAcct.add(s);
    CHECK(s.size() == 21);
    CHECK(strHex(s) == "140000000000000000000000000000000000000000");
    SerialIter sit(s.slice());
    STAccount const deserializedZero(sit, sfAccount);
    CHECK(deserializedZero.isEquivalent(zeroAcct));
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
    CHECK(regKey.isEquivalent(zeroAcct));
}

TEST_CASE("STAccount assignment")
{
    STAccount const defaultAcct;
    STAccount const zeroAcct{sfAccount, AccountID{}};

    STAccount assignAcct;
    CHECK(assignAcct.isEquivalent(defaultAcct));
    CHECK(assignAcct.isDefault());
    assignAcct = AccountID{};
    CHECK(!assignAcct.isEquivalent(defaultAcct));
    CHECK(assignAcct.isEquivalent(zeroAcct));
    CHECK(!assignAcct.isDefault());
}

TEST_CASE("AccountID parsing")
{
    auto const s = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";
    auto const parsed = parseBase58<AccountID>(s);
    REQUIRE(parsed);
    CHECK(toBase58(*parsed) == s);
}

TEST_CASE("AccountID invalid parsing")
{
    auto const s =
        "âabcd1rNxp4h8apvRis6mJf9Sh8C6iRxfrDWNâabcdAVâ\xc2\x80\xc2\x8f";
    CHECK(!parseBase58<AccountID>(s));
}

TEST_SUITE_END();
