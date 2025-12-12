#include <xrpl/json/Object.h>
#include <xrpl/json/Output.h>
#include <xrpl/json/Writer.h>

#include <doctest/doctest.h>

#include <memory>
#include <string>

using namespace Json;

TEST_SUITE_BEGIN("JsonObject");

struct ObjectFixture
{
    std::string output_;
    std::unique_ptr<WriterObject> writerObject_;

    Object&
    makeRoot()
    {
        output_.clear();
        writerObject_ =
            std::make_unique<WriterObject>(stringWriterObject(output_));
        return **writerObject_;
    }

    void
    expectResult(std::string const& expected)
    {
        writerObject_.reset();
        CHECK(output_ == expected);
    }
};

TEST_CASE_FIXTURE(ObjectFixture, "trivial")
{
    {
        auto& root = makeRoot();
        (void)root;
    }
    expectResult("{}");
}

TEST_CASE_FIXTURE(ObjectFixture, "simple")
{
    {
        auto& root = makeRoot();
        root["hello"] = "world";
        root["skidoo"] = 23;
        root["awake"] = false;
        root["temperature"] = 98.6;
    }

    expectResult(
        "{\"hello\":\"world\","
        "\"skidoo\":23,"
        "\"awake\":false,"
        "\"temperature\":98.6}");
}

TEST_CASE_FIXTURE(ObjectFixture, "oneSub")
{
    {
        auto& root = makeRoot();
        root.setArray("ar");
    }
    expectResult("{\"ar\":[]}");
}

TEST_CASE_FIXTURE(ObjectFixture, "subs")
{
    {
        auto& root = makeRoot();

        {
            // Add an array with three entries.
            auto array = root.setArray("ar");
            array.append(23);
            array.append(false);
            array.append(23.5);
        }

        {
            // Add an object with one entry.
            auto obj = root.setObject("obj");
            obj["hello"] = "world";
        }

        {
            // Add another object with two entries.
            Json::Value value;
            value["h"] = "w";
            value["f"] = false;
            root["obj2"] = value;
        }
    }

    // Json::Value has an unstable order...
    auto case1 =
        "{\"ar\":[23,false,23.5],"
        "\"obj\":{\"hello\":\"world\"},"
        "\"obj2\":{\"h\":\"w\",\"f\":false}}";
    auto case2 =
        "{\"ar\":[23,false,23.5],"
        "\"obj\":{\"hello\":\"world\"},"
        "\"obj2\":{\"f\":false,\"h\":\"w\"}}";
    writerObject_.reset();
    CHECK((output_ == case1 || output_ == case2));
}

TEST_CASE_FIXTURE(ObjectFixture, "subsShort")
{
    {
        auto& root = makeRoot();

        {
            // Add an array with three entries.
            auto array = root.setArray("ar");
            array.append(23);
            array.append(false);
            array.append(23.5);
        }

        // Add an object with one entry.
        root.setObject("obj")["hello"] = "world";

        {
            // Add another object with two entries.
            auto object = root.setObject("obj2");
            object.set("h", "w");
            object.set("f", false);
        }
    }
    expectResult(
        "{\"ar\":[23,false,23.5],"
        "\"obj\":{\"hello\":\"world\"},"
        "\"obj2\":{\"h\":\"w\",\"f\":false}}");
}

TEST_CASE_FIXTURE(ObjectFixture, "object failure")
{
    SUBCASE("object failure assign")
    {
        auto& root = makeRoot();
        auto obj = root.setObject("o1");
        CHECK_THROWS([&]() { root["fail"] = "complete"; }());
    }
    SUBCASE("object failure object")
    {
        auto& root = makeRoot();
        auto obj = root.setObject("o1");
        CHECK_THROWS([&]() { root.setObject("o2"); }());
    }
    SUBCASE("object failure Array")
    {
        auto& root = makeRoot();
        auto obj = root.setArray("o1");
        CHECK_THROWS([&]() { root.setArray("o2"); }());
    }
}

TEST_CASE_FIXTURE(ObjectFixture, "array failure")
{
    SUBCASE("array failure append")
    {
        auto& root = makeRoot();
        auto array = root.setArray("array");
        auto subarray = array.appendArray();
        auto fail = [&]() { array.append("fail"); };
        CHECK_THROWS(fail());
    }
    SUBCASE("array failure appendArray")
    {
        auto& root = makeRoot();
        auto array = root.setArray("array");
        auto subarray = array.appendArray();
        auto fail = [&]() { array.appendArray(); };
        CHECK_THROWS(fail());
    }
    SUBCASE("array failure appendObject")
    {
        auto& root = makeRoot();
        auto array = root.setArray("array");
        auto subarray = array.appendArray();
        auto fail = [&]() { array.appendObject(); };
        CHECK_THROWS(fail());
    }
}

TEST_CASE_FIXTURE(ObjectFixture, "repeating keys")
{
    auto& root = makeRoot();
    root.set("foo", "bar");
    root.set("baz", 0);
    // setting key again throws in !NDEBUG builds
    auto set_again = [&]() { root.set("foo", "bar"); };
#ifdef NDEBUG
    set_again();
    CHECK(true);  // pass
#else
    CHECK_THROWS(set_again());
#endif
}

TEST_SUITE_END();
