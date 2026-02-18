#include <test/jtx.h>
#include <test/jtx/Env.h>
#include <test/shamap/common.h>
#include <test/unit_test/SuiteJournal.h>

#include <xrpld/app/ledger/detail/LedgerNodeHelpers.h>

#include <xrpl/basics/random.h>
#include <xrpl/beast/unit_test.h>
#include <xrpl/proto/xrpl.pb.h>
#include <xrpl/shamap/SHAMap.h>
#include <xrpl/shamap/SHAMapItem.h>
#include <xrpl/shamap/SHAMapLeafNode.h>

namespace xrpl {
namespace tests {

class LedgerNodeHelpers_test : public beast::unit_test::suite
{
    // Helper to create a simple SHAMapItem for testing
    boost::intrusive_ptr<SHAMapItem>
    makeTestItem(std::uint32_t seed)
    {
        Serializer s;
        s.add32(seed);
        s.add32(seed + 1);
        s.add32(seed + 2);
        return make_shamapitem(s.getSHA512Half(), s.slice());
    }

    // Helper to serialize a tree node to wire format
    std::string
    serializeNode(intr_ptr::SharedPtr<SHAMapTreeNode> const& node)
    {
        Serializer s;
        node->serializeWithPrefix(s);
        auto const slice = s.slice();
        return std::string(reinterpret_cast<char const*>(slice.data()), slice.size());
    }

    void
    testValidateLedgerNode()
    {
        testcase("validateLedgerNode");

        using namespace test::jtx;

        /*// Test with amendment disabled.
        {
            Env env{*this, testable_amendments() - fixLedgerNodeID};
            auto& app = env.app();

            // Valid node with nodeid.
            protocol::TMLedgerNode node1;
            node1.set_nodedata("test_data");
            node1.set_nodeid("test_nodeid");
            BEAST_EXPECT(validateLedgerNode(app, node1));

            // Invalid: missing nodedata.
            protocol::TMLedgerNode node2;
            node2.set_nodeid("test_nodeid");
            BEAST_EXPECT(!validateLedgerNode(app, node2));

            // Invalid: has new field (id).
            protocol::TMLedgerNode node3;
            node3.set_nodedata("test_data");
            node3.set_nodeid("test_nodeid");
            node3.set_id("test_id");
            BEAST_EXPECT(!validateLedgerNode(app, node3));

            // Invalid: has new field (depth).
            protocol::TMLedgerNode node4;
            node4.set_nodedata("test_data");
            node4.set_nodeid("test_nodeid");
            node4.set_depth(5);
            BEAST_EXPECT(!validateLedgerNode(app, node4));
        }

        // Test with amendment enabled.
        {
            Env env{*this, testable_amendments() | fixLedgerNodeID};
            auto& app = env.app();

            // Valid inner node with id.
            protocol::TMLedgerNode node1;
            node1.set_nodedata("test_data");
            node1.set_id("test_id");
            BEAST_EXPECT(validateLedgerNode(app, node1));

            // Valid leaf node with depth.
            protocol::TMLedgerNode node2;
            node2.set_nodedata("test_data");
            node2.set_depth(5);
            BEAST_EXPECT(validateLedgerNode(app, node2));

            // Valid leaf node at max depth.
            protocol::TMLedgerNode node3;
            node3.set_nodedata("test_data");
            node3.set_depth(SHAMap::leafDepth);
            BEAST_EXPECT(validateLedgerNode(app, node3));

            // Invalid: depth exceeds max depth.
            protocol::TMLedgerNode node4;
            node4.set_nodedata("test_data");
            node4.set_depth(SHAMap::leafDepth + 1);
            BEAST_EXPECT(!validateLedgerNode(app, node4));

            // Invalid: has legacy field (nodeid).
            protocol::TMLedgerNode node5;
            node5.set_nodedata("test_data");
            node5.set_nodeid("test_nodeid");
            BEAST_EXPECT(!validateLedgerNode(app, node5));

            // Invalid: missing both id and depth.
            protocol::TMLedgerNode node6;
            node6.set_nodedata("test_data");
            BEAST_EXPECT(!validateLedgerNode(app, node6));

            // Invalid: missing nodedata.
            protocol::TMLedgerNode node7;
            node7.set_id("test_id");
            BEAST_EXPECT(!validateLedgerNode(app, node7));
        }*/
    }

    /*void
    testGetTreeNode()
    {
        testcase("getTreeNode");

        using namespace test::jtx;
        test::SuiteJournal journal("LedgerNodeHelpers_test", *this);
        TestNodeFamily f(journal);

        // Test with valid inner node
        {
            auto innerNode = std::make_shared<SHAMapInnerNode>(1);
            auto serialized = serializeNode(innerNode);
            auto result = getTreeNode(serialized);
            BEAST_EXPECT(result.has_value());
            BEAST_EXPECT((*result)->isInner());
        }

        // Test with valid leaf node
        {
            auto item = makeTestItem(12345);
            auto leafNode =
                std::make_shared<SHAMapLeafNode>(item, SHAMapNodeType::tnACCOUNT_STATE);
            auto serialized = serializeNode(leafNode);
            auto result = getTreeNode(serialized);
            BEAST_EXPECT(result.has_value());
            BEAST_EXPECT((*result)->isLeaf());
        }

        // Test with invalid data - empty string
        {
            auto result = getTreeNode("");
            BEAST_EXPECT(!result.has_value());
        }

        // Test with invalid data - garbage data
        {
            std::string garbage = "This is not a valid serialized node!";
            auto result = getTreeNode(garbage);
            BEAST_EXPECT(!result.has_value());
        }

        // Test with malformed data - truncated
        {
            auto item = makeTestItem(54321);
            auto leafNode =
                std::make_shared<SHAMapLeafNode>(item, SHAMapNodeType::tnACCOUNT_STATE);
            auto serialized = serializeNode(leafNode);
            // Truncate the data
            serialized = serialized.substr(0, 5);
            auto result = getTreeNode(serialized);
            BEAST_EXPECT(!result.has_value());
        }
    }

    void
    testGetSHAMapNodeID()
    {
        testcase("getSHAMapNodeID");

        using namespace test::jtx;
        test::SuiteJournal journal("LedgerNodeHelpers_test", *this);
        TestNodeFamily f(journal);

        auto item = makeTestItem(99999);
        auto leafNode =
            std::make_shared<SHAMapLeafNode>(item, SHAMapNodeType::tnACCOUNT_STATE);
        auto innerNode = std::make_shared<SHAMapInnerNode>(1);

        // Test with amendment disabled
        {
            Env env{*this};
            auto& app = env.app();

            // Test with leaf node - valid nodeid
            {
                auto nodeID = SHAMapNodeID::createID(5, item->key());
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");
                ledgerNode.set_nodeid(nodeID.getRawString());

                auto result = getSHAMapNodeID(app, ledgerNode, leafNode);
                BEAST_EXPECT(result.has_value());
                BEAST_EXPECT(*result == nodeID);
            }

            // Test with leaf node - invalid nodeid (wrong depth)
            {
                auto nodeID = SHAMapNodeID::createID(7, item->key());
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");
                ledgerNode.set_nodeid(nodeID.getRawString());

                auto wrongNodeID = SHAMapNodeID::createID(5, item->key());
                ledgerNode.set_nodeid(wrongNodeID.getRawString());

                auto result = getSHAMapNodeID(app, ledgerNode, leafNode);
                // Should fail because nodeid doesn't match the leaf's key at the
                // given depth
                BEAST_EXPECT(!result.has_value());
            }

            // Test with inner node
            {
                auto nodeID = SHAMapNodeID::createID(3, uint256{});
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");
                ledgerNode.set_nodeid(nodeID.getRawString());

                auto result = getSHAMapNodeID(app, ledgerNode, innerNode);
                BEAST_EXPECT(result.has_value());
                BEAST_EXPECT(*result == nodeID);
            }

            // Test missing nodeid
            {
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");

                auto result = getSHAMapNodeID(app, ledgerNode, leafNode);
                BEAST_EXPECT(!result.has_value());
            }
        }

        // Test with amendment enabled
        {
            Env env{*this, testable_amendments() | fixLedgerNodeID};
            auto& app = env.app();

            // Test with leaf node - valid depth
            {
                std::uint32_t depth = 5;
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");
                ledgerNode.set_depth(depth);

                auto result = getSHAMapNodeID(app, ledgerNode, leafNode);
                BEAST_EXPECT(result.has_value());
                auto expectedID = SHAMapNodeID::createID(depth, item->key());
                BEAST_EXPECT(*result == expectedID);
            }

            // Test with leaf node - missing depth
            {
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");

                auto result = getSHAMapNodeID(app, ledgerNode, leafNode);
                BEAST_EXPECT(!result.has_value());
            }

            // Test with inner node - valid id
            {
                auto nodeID = SHAMapNodeID::createID(3, uint256{});
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");
                ledgerNode.set_id(nodeID.getRawString());

                auto result = getSHAMapNodeID(app, ledgerNode, innerNode);
                BEAST_EXPECT(result.has_value());
                BEAST_EXPECT(*result == nodeID);
            }

            // Test with inner node - missing id
            {
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");

                auto result = getSHAMapNodeID(app, ledgerNode, innerNode);
                BEAST_EXPECT(!result.has_value());
            }

            // Test that nodeid is rejected when amendment is enabled
            {
                auto nodeID = SHAMapNodeID::createID(5, item->key());
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");
                ledgerNode.set_nodeid(nodeID.getRawString());

                // Should fail validation before getSHAMapNodeID is called,
                // but let's verify getSHAMapNodeID behavior
                // Note: validateLedgerNode should be called first in practice
                BEAST_EXPECT(!validateLedgerNode(app, ledgerNode));
            }

            // Test with root node (inner node at depth 0)
            {
                auto rootNode = std::make_shared<SHAMapInnerNode>(1);
                auto nodeID = SHAMapNodeID{};  // root node ID
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");
                ledgerNode.set_id(nodeID.getRawString());

                auto result = getSHAMapNodeID(app, ledgerNode, rootNode);
                BEAST_EXPECT(result.has_value());
                BEAST_EXPECT(*result == nodeID);
            }

            // Test with leaf at maximum depth
            {
                std::uint32_t depth = SHAMap::leafDepth;
                protocol::TMLedgerNode ledgerNode;
                ledgerNode.set_nodedata("test_data");
                ledgerNode.set_depth(depth);

                auto result = getSHAMapNodeID(app, ledgerNode, leafNode);
                BEAST_EXPECT(result.has_value());
                auto expectedID = SHAMapNodeID::createID(depth, item->key());
                BEAST_EXPECT(*result == expectedID);
            }
        }
    }*/

public:
    void
    run() override
    {
        testValidateLedgerNode();
        // testGetTreeNode();
        // testGetSHAMapNodeID();
    }
};

BEAST_DEFINE_TESTSUITE(LedgerNodeHelpers, app, xrpl);

}  // namespace tests
}  // namespace xrpl
