#include <test/csf/Histogram.h>

#include <doctest/doctest.h>

using namespace xrpl::test::csf;

TEST_SUITE_BEGIN("Histogram");

TEST_CASE("Histogram empty")
{
    Histogram<int> hist;

    CHECK(hist.size() == 0);
    CHECK(hist.numBins() == 0);
    CHECK(hist.minValue() == 0);
    CHECK(hist.maxValue() == 0);
    CHECK(hist.avg() == 0);
    CHECK(hist.percentile(0.0f) == hist.minValue());
    CHECK(hist.percentile(0.5f) == 0);
    CHECK(hist.percentile(0.9f) == 0);
    CHECK(hist.percentile(1.0f) == hist.maxValue());
}

TEST_CASE("Histogram single element")
{
    Histogram<int> hist;
    hist.insert(1);

    CHECK(hist.size() == 1);
    CHECK(hist.numBins() == 1);
    CHECK(hist.minValue() == 1);
    CHECK(hist.maxValue() == 1);
    CHECK(hist.avg() == 1);
    CHECK(hist.percentile(0.0f) == hist.minValue());
    CHECK(hist.percentile(0.5f) == 1);
    CHECK(hist.percentile(0.9f) == 1);
    CHECK(hist.percentile(1.0f) == hist.maxValue());
}

TEST_CASE("Histogram two elements")
{
    Histogram<int> hist;
    hist.insert(1);
    hist.insert(9);

    CHECK(hist.size() == 2);
    CHECK(hist.numBins() == 2);
    CHECK(hist.minValue() == 1);
    CHECK(hist.maxValue() == 9);
    CHECK(hist.avg() == 5);
    CHECK(hist.percentile(0.0f) == hist.minValue());
    CHECK(hist.percentile(0.5f) == 1);
    CHECK(hist.percentile(0.9f) == 9);
    CHECK(hist.percentile(1.0f) == hist.maxValue());
}

TEST_CASE("Histogram duplicate elements")
{
    Histogram<int> hist;
    hist.insert(1);
    hist.insert(9);
    hist.insert(1);

    CHECK(hist.size() == 3);
    CHECK(hist.numBins() == 2);
    CHECK(hist.minValue() == 1);
    CHECK(hist.maxValue() == 9);
    CHECK(hist.avg() == 11 / 3);
    CHECK(hist.percentile(0.0f) == hist.minValue());
    CHECK(hist.percentile(0.5f) == 1);
    CHECK(hist.percentile(0.9f) == 9);
    CHECK(hist.percentile(1.0f) == hist.maxValue());
}

TEST_SUITE_END();

