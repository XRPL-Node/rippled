#include <xrpl/basics/Number.h>
#include <xrpl/beast/unit_test.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/Issue.h>
#include <xrpl/protocol/MPTIssue.h>
#include <xrpl/protocol/STAmount.h>

namespace xrpl {

// Result types for vault operations
struct DepositResult
{
    STAmount shares;
    STAmount assets;
};

struct WithdrawResult
{
    STAmount shares;
    STAmount assets;
};

// Proof-of-concept Vault implementing the XLS-0065 share pricing model.
// Uses STAmount for typed asset/share values and Number for arithmetic.
class Vault
{
    Asset asset_;
    MPTIssue shareAsset_;
    std::uint8_t scale_;
    Number assetsTotal_{0};
    Number assetsAvailable_{0};
    Number sharesTotal_{0};
    Number interestUnrealized_{0};
    Number lossUnrealized_{0};

    static inline std::uint32_t nextShareSeq_ = 1;

public:
    Vault(Asset asset, std::uint8_t scale = 6)
        : asset_(asset)
        , shareAsset_(makeMptID(nextShareSeq_++, AccountID(0xFACE)))
        , scale_(asset.native() || asset.integral() ? 0 : scale)
    {
        // XRP and MPT force scale to 0 per spec
        // IOU uses caller-provided scale (default 6)
    }

    DepositResult
    deposit(STAmount const& assets)
    {
        Number const depositAmount = assets;

        Number shares;
        Number actualAssets;

        if (assetsTotal_ == 0 && sharesTotal_ == 0)
        {
            // Initial deposit: shares = assets * 10^scale
            shares = Number(depositAmount.mantissa(), depositAmount.exponent() + scale_).truncate();
            actualAssets = depositAmount;
        }
        else
        {
            // Subsequent deposit: shares = floor(assets * sharesTotal / depositNAV)
            Number const depositNAV = assetsTotal_ - interestUnrealized_;
            shares = ((depositAmount * sharesTotal_) / depositNAV).truncate();

            // Recalculate actual assets taken
            actualAssets = (shares * depositNAV) / sharesTotal_;
        }

        assetsTotal_ += actualAssets;
        assetsAvailable_ += actualAssets;
        sharesTotal_ += shares;

        return {
            STAmount{shareAsset_, shares},
            STAmount{asset_, actualAssets},
        };
    }

    STAmount
    redeem(STAmount const& shares)
    {
        Number const shareAmount = shares;

        // assets = shares * withdrawalNAV / sharesTotal
        Number const withdrawalNAV = assetsTotal_ - interestUnrealized_ - lossUnrealized_;
        Number const assetsOut = (shareAmount * withdrawalNAV) / sharesTotal_;

        assetsTotal_ -= assetsOut;
        assetsAvailable_ -= assetsOut;
        sharesTotal_ -= shareAmount;

        return STAmount{asset_, assetsOut};
    }

    WithdrawResult
    withdraw(STAmount const& assetsRequested)
    {
        Number const requestedAmount = assetsRequested;

        Number const withdrawalNAV = assetsTotal_ - interestUnrealized_ - lossUnrealized_;

        // shares = round_nearest(requested * sharesTotal / withdrawalNAV)
        Number const rawShares = (requestedAmount * sharesTotal_) / withdrawalNAV;
        // Number::operator int64_t() rounds to nearest (not standard C++ truncation)
        Number const shares{static_cast<std::int64_t>(rawShares)};

        // Recalculate actual assets out
        Number const assetsOut = (shares * withdrawalNAV) / sharesTotal_;

        assetsTotal_ -= assetsOut;
        assetsAvailable_ -= assetsOut;
        sharesTotal_ -= shares;

        return {
            STAmount{shareAsset_, shares},
            STAmount{asset_, assetsOut},
        };
    }

    void
    setInterestUnrealized(Number omega)
    {
        interestUnrealized_ = omega;
    }

    void
    setLossUnrealized(Number iota)
    {
        lossUnrealized_ = iota;
    }

    void
    addRealizedInterest(Number amount)
    {
        assetsTotal_ += amount;
        assetsAvailable_ += amount;
    }

    STAmount
    assetsTotal() const
    {
        return STAmount{asset_, assetsTotal_};
    }

    STAmount
    assetsAvailable() const
    {
        return STAmount{asset_, assetsAvailable_};
    }

    STAmount
    sharesTotal() const
    {
        return STAmount{shareAsset_, sharesTotal_};
    }

    MPTIssue const&
    shareAsset() const
    {
        return shareAsset_;
    }

    Number
    interestUnrealized() const
    {
        return interestUnrealized_;
    }

    Number
    lossUnrealized() const
    {
        return lossUnrealized_;
    }
};

class VaultSharePricing_test : public beast::unit_test::suite
{
public:
    void
    testInitialDepositIOU()
    {
        testcase("Initial deposit IOU (scale=6)");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd};  // scale defaults to 6

        auto const [shares, assets] = vault.deposit(STAmount{usd, 100});

        // 100 * 10^6 = 100,000,000 shares
        BEAST_EXPECT(Number(shares) == Number(100'000'000));
        BEAST_EXPECT(Number(assets) == Number(100));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(100));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(100'000'000));
    }

    void
    testInitialDepositXRP()
    {
        testcase("Initial deposit XRP (scale=0)");

        Vault vault{xrpIssue()};

        // 10 XRP = 10,000,000 drops; scale=0 so shares = drops 1:1
        auto const [shares, assets] = vault.deposit(STAmount{xrpIssue(), 10'000'000});

        BEAST_EXPECT(Number(shares) == Number(10'000'000));
        BEAST_EXPECT(Number(assets) == Number(10'000'000));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(10'000'000));
    }

    void
    testInitialDepositMPT()
    {
        testcase("Initial deposit MPT (scale=0)");

        MPTIssue const mptAsset{makeMptID(100, AccountID(0x5678))};
        Vault vault{mptAsset};

        auto const [shares, assets] = vault.deposit(STAmount{mptAsset, 500});

        // scale=0, so 500 assets → 500 shares
        BEAST_EXPECT(Number(shares) == Number(500));
        BEAST_EXPECT(Number(assets) == Number(500));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(500));
    }

    void
    testSubsequentDeposit()
    {
        testcase("Subsequent deposit (proportional)");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd};

        // Initial: 100 assets → 100M shares
        vault.deposit(STAmount{usd, 100});

        // Second deposit: 50 assets at same price → 50M shares
        auto const [shares, assets] = vault.deposit(STAmount{usd, 50});

        BEAST_EXPECT(Number(shares) == Number(50'000'000));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(150));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(150'000'000));
    }

    void
    testRedeemBasic()
    {
        testcase("Redeem basic");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd};

        vault.deposit(STAmount{usd, 100});

        // Redeem half the shares
        auto const assetsOut = vault.redeem(STAmount{vault.shareAsset(), 50'000'000});

        BEAST_EXPECT(Number(assetsOut) == Number(50));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(50));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(50'000'000));
    }

    void
    testWithdrawBasic()
    {
        testcase("Withdraw basic");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd};

        vault.deposit(STAmount{usd, 100});

        // Withdraw 50 assets
        auto const [shares, assets] = vault.withdraw(STAmount{usd, 50});

        BEAST_EXPECT(Number(shares) == Number(50'000'000));
        BEAST_EXPECT(Number(assets) == Number(50));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(50));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(50'000'000));
    }

    void
    testAsymmetricDepositWithInterest()
    {
        testcase("Asymmetric pricing - deposit with unrealized interest");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};  // scale=0 for simpler math

        // Seed vault: 1000 assets, 1000 shares
        vault.deposit(STAmount{usd, 1000});
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1000));

        // Protocol adds unrealized interest (omega = 50)
        vault.setInterestUnrealized(Number(50));

        // Deposit NAV = assetsTotal - omega = 1000 - 50 = 950
        // Deposit 95 assets: shares = floor(95 * 1000 / 950) = floor(100) = 100
        auto const [shares, assets] = vault.deposit(STAmount{usd, 95});

        BEAST_EXPECT(Number(shares) == Number(100));
        // Recalculated assets: 100 * 950 / 1000 = 95
        BEAST_EXPECT(Number(assets) == Number(95));
    }

    void
    testAsymmetricWithdrawWithLoss()
    {
        testcase("Asymmetric pricing - withdraw with unrealized loss");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};

        vault.deposit(STAmount{usd, 1000});
        vault.setInterestUnrealized(Number(50));
        vault.setLossUnrealized(Number(100));

        // Withdrawal NAV = 1000 - 50 - 100 = 850
        // Redeem 100 shares: assets = 100 * 850 / 1000 = 85
        auto const assetsOut = vault.redeem(STAmount{vault.shareAsset(), 100});

        BEAST_EXPECT(Number(assetsOut) == Number(85));
    }

    void
    testSpecExample()
    {
        testcase("Spec example (section 3.1.7.1)");

        // Vault: assetsTotal=1000, omega=50, iota=100, sharesTotal=1000
        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};

        vault.deposit(STAmount{usd, 1000});
        vault.setInterestUnrealized(Number(50));
        vault.setLossUnrealized(Number(100));

        // Deposit share price = (assetsTotal - omega) / sharesTotal
        //                     = (1000 - 50) / 1000 = 0.95
        // So 1 share costs 0.95 assets to deposit
        // Depositing 0.95 assets: shares = floor(0.95 * 1000 / 950) = floor(1) = 1
        {
            auto const [shares, assets] = vault.deposit(STAmount{usd, 95, -2});  // 0.95
            BEAST_EXPECT(Number(shares) == Number(1));
        }

        // After deposit: assetsTotal = 1000.95, sharesTotal = 1001
        // withdrawalNAV = assetsTotal - omega - iota = 1000.95 - 50 - 100 = 850.95
        // Redeem 1 share: assetsOut = 1 * 850.95 / 1001
        {
            Number const expected = Number(85095, -2) / Number(1001);
            STAmount const expectedAmt(usd, expected);
            auto const redeemAssets = vault.redeem(STAmount{vault.shareAsset(), 1});
            BEAST_EXPECT(redeemAssets == expectedAmt);
        }
    }

    void
    testDepositRoundingDown()
    {
        testcase("Deposit rounding - shares floor");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault2{usd, 0};
        vault2.deposit(STAmount{usd, 10});
        // 10 assets, 10 shares
        // Deposit 3: shares = floor(3 * 10 / 10) = 3 (exact)
        // Need non-trivial ratio. Let's add interest to skew the price.
        vault2.setInterestUnrealized(Number(3));
        // depositNAV = 10 - 3 = 7
        // Deposit 1: shares = floor(1 * 10 / 7) = floor(1.4285...) = 1
        auto const [shares, assets] = vault2.deposit(STAmount{usd, 1});
        BEAST_EXPECT(Number(shares) == Number(1));
        // Recalculated assets: 1 * 7 / 10 = 0.7
        BEAST_EXPECT(Number(assets) == Number(7, -1));
    }

    void
    testWithdrawRoundingNearest()
    {
        testcase("Withdraw rounding - shares nearest");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};

        vault.deposit(STAmount{usd, 10});
        vault.setInterestUnrealized(Number(3));
        // withdrawalNAV = 10 - 3 = 7
        // Withdraw 1 asset: shares = round(1 * 10 / 7) = round(1.4285...) = 1
        auto const [shares, assets] = vault.withdraw(STAmount{usd, 1});
        BEAST_EXPECT(Number(shares) == Number(1));

        // Round up case: rawShares = 1.1*10/7 ≈ 1.571 → nearest = 2
        Vault vault2{usd, 0};
        vault2.deposit(STAmount{usd, 10});
        vault2.setInterestUnrealized(Number(3));
        auto const [shares2, assets2] = vault2.withdraw(STAmount{usd, 11, -1});  // 1.1
        BEAST_EXPECT(Number(shares2) == Number(2));
        // assetsOut = 2*7/10 = 1.4
        BEAST_EXPECT(Number(assets2) == Number(14, -1));
    }

    void
    testRedeemAll()
    {
        testcase("Redeem all shares - vault empties");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd};

        vault.deposit(STAmount{usd, 100});
        auto const assetsOut = vault.redeem(STAmount{vault.shareAsset(), 100'000'000});

        BEAST_EXPECT(Number(assetsOut) == Number(100));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(0));
        BEAST_EXPECT(Number(vault.assetsAvailable()) == Number(0));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(0));
    }

    void
    testLossOnlyVault()
    {
        testcase("Vault with lossUnrealized only (no interestUnrealized)");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};

        vault.deposit(STAmount{usd, 1000});
        vault.setLossUnrealized(Number(200));

        // Deposit NAV = 1000 - 0 = 1000 (loss doesn't affect deposit)
        auto const [depShares, depAssets] = vault.deposit(STAmount{usd, 100});
        BEAST_EXPECT(Number(depShares) == Number(100));

        // Withdrawal NAV = 1100 - 0 - 200 = 900 (total is now 1100)
        // Redeem 100 shares: assets = 100 * 900 / 1100 ≈ 81.818...
        auto const redeemAssets = vault.redeem(STAmount{vault.shareAsset(), 100});
        // 100 * 900 / 1100 = 81.8181...
        // Compare as STAmount to account for IOU normalization
        Number const expected = (Number(100) * Number(900)) / Number(1100);
        STAmount const expectedAmt(usd, expected);
        BEAST_EXPECT(redeemAssets == expectedAmt);
    }

    void
    testMultipleDepositorsIOU()
    {
        testcase("Multiple depositors with yield (IOU)");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};  // scale=0 for clean integer math

        // User A deposits 100 → 100 shares
        auto const [sharesA, assetsA] = vault.deposit(STAmount{usd, 100});
        BEAST_EXPECT(Number(sharesA) == Number(100));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(100));

        // Protocol realizes 10 interest: assetsTotal becomes 110
        vault.addRealizedInterest(Number(10));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(110));

        // User B deposits 100: depositNAV=110, shares=floor(100*100/110)=90
        // actualAssets = 90*110/100 = 99
        auto const [sharesB, assetsB] = vault.deposit(STAmount{usd, 100});
        BEAST_EXPECT(Number(sharesB) == Number(90));
        BEAST_EXPECT(Number(assetsB) == Number(99));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(209));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(190));

        // A redeems 100 shares: assetsOut = 100 * 209 / 190 = 110
        auto const outA = vault.redeem(STAmount{vault.shareAsset(), 100});
        BEAST_EXPECT(Number(outA) == Number(110));

        // B redeems 90 shares: assetsOut = 90 * 99 / 90 = 99
        auto const outB = vault.redeem(STAmount{vault.shareAsset(), 90});
        BEAST_EXPECT(Number(outB) == Number(99));

        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(0));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(0));
    }

    void
    testPrecisionLoss()
    {
        testcase("Precision loss - zero shares condition");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};

        // Initial deposit: 1 asset → 1 share
        vault.deposit(STAmount{usd, 1});
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1));

        // Deposit 0.0001 assets: shares = floor(0.0001 * 1 / 1) = 0
        // This is the zero-shares condition (tecPRECISION_LOSS in real implementation)
        auto const [shares, assets] = vault.deposit(STAmount{usd, 1, -4});
        BEAST_EXPECT(Number(shares) == Number(0));
    }

    void
    testXRPFullCycle()
    {
        testcase("XRP full cycle: deposit, withdraw, redeem");

        Vault vault{xrpIssue()};

        // Deposit 10 XRP = 10M drops
        auto const [shares1, assets1] = vault.deposit(STAmount{xrpIssue(), 10'000'000});
        BEAST_EXPECT(Number(shares1) == Number(10'000'000));

        // Withdraw 5M drops
        auto const [wShares, wAssets] = vault.withdraw(STAmount{xrpIssue(), 5'000'000});
        BEAST_EXPECT(Number(wShares) == Number(5'000'000));
        BEAST_EXPECT(Number(wAssets) == Number(5'000'000));

        // Redeem remaining 5M shares
        auto const rAssets = vault.redeem(STAmount{vault.shareAsset(), 5'000'000});
        BEAST_EXPECT(Number(rAssets) == Number(5'000'000));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(0));
        BEAST_EXPECT(Number(vault.assetsAvailable()) == Number(0));
    }

    void
    testMPTFullCycle()
    {
        testcase("MPT full cycle: deposit, withdraw, redeem");

        MPTIssue const mptAsset{makeMptID(200, AccountID(0x5678))};
        Vault vault{mptAsset};

        auto const [shares1, assets1] = vault.deposit(STAmount{mptAsset, 1000});
        BEAST_EXPECT(Number(shares1) == Number(1000));

        auto const [wShares, wAssets] = vault.withdraw(STAmount{mptAsset, 400});
        BEAST_EXPECT(Number(wShares) == Number(400));

        auto const rAssets = vault.redeem(STAmount{vault.shareAsset(), 600});
        BEAST_EXPECT(Number(rAssets) == Number(600));
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(0));
    }

    void
    testTinyDepositIntoLargeVault()
    {
        testcase("Tiny deposit into large vault (IOU)");

        // Large vault with 1 billion assets at scale=6
        // sharesTotal = 1e9 * 1e6 = 1e15
        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd};  // scale=6

        vault.deposit(STAmount{usd, UINT64_C(1'000'000'000)});
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1, 15));

        // Tiny deposit: 0.000001 (1e-6) into a 1e9 vault
        // shares = floor(1e-6 * 1e15 / 1e9) = floor(1e0) = 1
        auto const [shares, assets] =
            vault.deposit(STAmount{usd, 1, -6});

        BEAST_EXPECT(Number(shares) == Number(1));
        // Recalculated assets: 1 * 1e9 / 1e15 = 1e-6
        BEAST_EXPECT(Number(assets) == Number(1, -6));

        // Redeem the 1 share back
        auto const out = vault.redeem(STAmount{vault.shareAsset(), 1});
        BEAST_EXPECT(Number(out) == Number(1, -6));
    }

    void
    testTinyDepositIntoLargeVaultXRP()
    {
        testcase("Tiny deposit into large vault (XRP)");

        // 100 million XRP = 1e14 drops, scale=0 so shares = 1e14
        Vault vault{xrpIssue()};

        vault.deposit(STAmount{xrpIssue(), UINT64_C(100'000'000'000'000)});
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1, 14));

        // Deposit 1 drop into a 1e14-drop vault
        // shares = floor(1 * 1e14 / 1e14) = 1
        auto const [shares, assets] =
            vault.deposit(STAmount{xrpIssue(), 1});

        BEAST_EXPECT(Number(shares) == Number(1));
        BEAST_EXPECT(Number(assets) == Number(1));

        // Redeem the 1 share
        auto const out = vault.redeem(STAmount{vault.shareAsset(), 1});
        BEAST_EXPECT(Number(out) == Number(1));
    }

    void
    testLargeDepositIntoTinyVault()
    {
        testcase("Large deposit into tiny vault (IOU)");

        // Tiny vault: 0.001 assets, scale=6 → shares = 0.001 * 1e6 = 1000
        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd};

        vault.deposit(STAmount{usd, 1, -3});
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1000));

        // Massive deposit: 1 billion assets
        // shares = floor(1e9 * 1000 / 0.001) = floor(1e15) = 1e15
        auto const [shares, assets] =
            vault.deposit(STAmount{usd, UINT64_C(1'000'000'000)});

        BEAST_EXPECT(Number(shares) == Number(1, 15));
        BEAST_EXPECT(Number(assets) == Number(1, 9));

        // Vault now: assetsTotal ≈ 1e9, sharesTotal = 1e15 + 1000
        // Redeem original 1000 shares
        // assetsOut = 1000 * assetsTotal / sharesTotal
        // The original depositor should get back roughly 0.001
        auto const out = vault.redeem(STAmount{vault.shareAsset(), 1000});
        // 1000 * (1e9 + 0.001) / (1e15 + 1000) ≈ 0.001
        // Precision loss means the exact value depends on Number rounding
        Number const expected =
            (Number(1000) * Number(vault.assetsTotal())) /
            (Number(vault.sharesTotal()) + Number(out));
        STAmount const expectedAmt(usd, expected);
        BEAST_EXPECT(out == expectedAmt);
    }

    void
    testLargeDepositIntoTinyVaultXRP()
    {
        testcase("Large deposit into tiny vault (XRP)");

        // Tiny vault: 1 drop, scale=0 → 1 share
        Vault vault{xrpIssue()};

        vault.deposit(STAmount{xrpIssue(), 1});
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1));

        // Massive deposit: 1e14 drops (100M XRP)
        // shares = floor(1e14 * 1 / 1) = 1e14
        auto const [shares, assets] =
            vault.deposit(STAmount{xrpIssue(), UINT64_C(100'000'000'000'000)});

        BEAST_EXPECT(Number(shares) == Number(1, 14));
        BEAST_EXPECT(Number(assets) == Number(1, 14));

        // Redeem original 1 share: assetsOut = 1 * (1e14+1) / (1e14+1) = 1
        auto const out = vault.redeem(STAmount{vault.shareAsset(), 1});
        BEAST_EXPECT(Number(out) == Number(1));
    }

    void
    testHighPrecisionWithInterest()
    {
        testcase("High precision deposit/redeem with unrealized interest");

        // Large vault with interest: tests that tiny depositors aren't
        // short-changed or over-paid when NAV diverges from 1:1
        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};  // scale=0

        // Seed: 1e12 assets → 1e12 shares
        vault.deposit(STAmount{usd, UINT64_C(1'000'000'000'000)});

        // Large unrealized interest: 50% of total
        vault.setInterestUnrealized(Number(5, 11));
        // depositNAV = 1e12 - 5e11 = 5e11

        // Tiny deposit: 1 asset
        // shares = floor(1 * 1e12 / 5e11) = floor(2) = 2
        auto const [shares, assets] = vault.deposit(STAmount{usd, 1});
        BEAST_EXPECT(Number(shares) == Number(2));
        // Recalculated assets: 2 * 5e11 / 1e12 = 1
        BEAST_EXPECT(Number(assets) == Number(1));

        // Redeem those 2 shares
        // withdrawalNAV = (1e12 + 1) - 5e11 - 0 = 5e11 + 1
        // assetsOut = 2 * (5e11 + 1) / (1e12 + 2)
        auto const out = vault.redeem(STAmount{vault.shareAsset(), 2});
        // Should get back ≈ 1 (within rounding)
        Number const expectedNum =
            (Number(2) * (Number(1, 12) + Number(1) - Number(5, 11))) /
            (Number(1, 12) + Number(2));
        STAmount const expectedAmt(usd, expectedNum);
        BEAST_EXPECT(out == expectedAmt);
    }

    void
    testHighPrecisionWithLoss()
    {
        testcase("High precision redeem with unrealized loss");

        // Large vault with loss: tests that a tiny share redemption
        // correctly accounts for the discounted withdrawalNAV
        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};

        vault.deposit(STAmount{usd, UINT64_C(1'000'000'000'000)});

        // 20% unrealized loss
        vault.setLossUnrealized(Number(2, 11));
        // withdrawalNAV = 1e12 - 0 - 2e11 = 8e11

        // Redeem 1 share out of 1e12
        // assetsOut = 1 * 8e11 / 1e12 = 0.8
        auto const out = vault.redeem(STAmount{vault.shareAsset(), 1});
        Number const expectedNum = Number(8, 11) / Number(1, 12);
        STAmount const expectedAmt(usd, expectedNum);
        BEAST_EXPECT(out == expectedAmt);
    }

    void
    testManyTinyDepositsLargeVault()
    {
        testcase("Many tiny deposits into large vault then full redeem");

        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};  // scale=0

        // Seed with 1e9 assets
        vault.deposit(STAmount{usd, UINT64_C(1'000'000'000)});

        Number totalTinyShares{0};
        Number totalTinyAssets{0};

        // 100 tiny deposits of 1 asset each
        for (int i = 0; i < 100; ++i)
        {
            auto const [s, a] = vault.deposit(STAmount{usd, 1});
            totalTinyShares += Number(s);
            totalTinyAssets += Number(a);
        }

        // Each deposit: shares = floor(1 * sharesTotal / assetsTotal)
        // First tiny: floor(1 * 1e9 / 1e9) = 1 share per deposit
        BEAST_EXPECT(totalTinyShares == Number(100));
        BEAST_EXPECT(totalTinyAssets == Number(100));

        // Redeem all 100 tiny shares
        auto const out = vault.redeem(STAmount{vault.shareAsset(), 100});
        // assetsOut = 100 * (1e9 + 100) / (1e9 + 100) = 100
        BEAST_EXPECT(Number(out) == Number(100));
    }

    void
    testExtremeLiquidityRatioIOU()
    {
        testcase("Extreme liquidity ratio - 1e15 to 1 (IOU)");

        // Vault with maximum practical IOU assets at scale=0
        Issue const usd{Currency(0x5553440000000000), AccountID(0x4985601)};
        Vault vault{usd, 0};

        // 1e15 assets → 1e15 shares
        vault.deposit(STAmount{usd, UINT64_C(1'000'000'000'000'000)});
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1, 15));

        // Deposit 1 asset: shares = floor(1 * 1e15 / 1e15) = 1
        auto const [shares, assets] = vault.deposit(STAmount{usd, 1});
        BEAST_EXPECT(Number(shares) == Number(1));
        BEAST_EXPECT(Number(assets) == Number(1));

        // Redeem that 1 share
        auto const out = vault.redeem(STAmount{vault.shareAsset(), 1});
        BEAST_EXPECT(Number(out) == Number(1));

        // Vault should be back to exactly 1e15
        BEAST_EXPECT(Number(vault.assetsTotal()) == Number(1, 15));
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1, 15));
    }

    void
    testExtremeLiquidityRatioMPT()
    {
        testcase("Extreme liquidity ratio - 1e15 to 1 (MPT)");

        MPTIssue const mptAsset{makeMptID(300, AccountID(0x5678))};
        Vault vault{mptAsset};

        vault.deposit(STAmount{mptAsset, UINT64_C(1'000'000'000'000'000)});
        BEAST_EXPECT(Number(vault.sharesTotal()) == Number(1, 15));

        auto const [shares, assets] = vault.deposit(STAmount{mptAsset, 1});
        BEAST_EXPECT(Number(shares) == Number(1));

        auto const out = vault.redeem(STAmount{vault.shareAsset(), 1});
        BEAST_EXPECT(Number(out) == Number(1));
    }

    void
    run() override
    {
        testInitialDepositIOU();
        testInitialDepositXRP();
        testInitialDepositMPT();
        testSubsequentDeposit();
        testRedeemBasic();
        testWithdrawBasic();
        testAsymmetricDepositWithInterest();
        testAsymmetricWithdrawWithLoss();
        testSpecExample();
        testDepositRoundingDown();
        testWithdrawRoundingNearest();
        testRedeemAll();
        testLossOnlyVault();
        testMultipleDepositorsIOU();
        testPrecisionLoss();
        testXRPFullCycle();
        testMPTFullCycle();
        testTinyDepositIntoLargeVault();
        testTinyDepositIntoLargeVaultXRP();
        testLargeDepositIntoTinyVault();
        testLargeDepositIntoTinyVaultXRP();
        testHighPrecisionWithInterest();
        testHighPrecisionWithLoss();
        testManyTinyDepositsLargeVault();
        testExtremeLiquidityRatioIOU();
        testExtremeLiquidityRatioMPT();
    }
};

BEAST_DEFINE_TESTSUITE(VaultSharePricing, basics, xrpl);

}  // namespace xrpl
