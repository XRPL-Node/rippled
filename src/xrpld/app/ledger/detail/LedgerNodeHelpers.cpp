#include <xrpld/app/ledger/detail/LedgerNodeHelpers.h>
#include <xrpld/app/main/Application.h>

#include <xrpl/basics/IntrusivePointer.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/ledger/AmendmentTable.h>
#include <xrpl/shamap/SHAMapLeafNode.h>
#include <xrpl/shamap/SHAMapNodeID.h>
#include <xrpl/shamap/SHAMapTreeNode.h>

namespace xrpl {

bool
validateLedgerNode(Application& app, protocol::TMLedgerNode const& ledger_node)
{
    if (!ledger_node.has_nodedata())
        return false;

    if (app.getAmendmentTable().isEnabled(fixLedgerNodeID))
    {
        // Note that we cannot confirm here whether the node is actually an
        // inner or leaf node, and will need to perform additional checks
        // separately.
        if (ledger_node.has_nodeid())
            return false;
        if (ledger_node.has_id())
            return true;
        return ledger_node.has_depth() && ledger_node.depth() <= SHAMap::leafDepth;
    }

    if (ledger_node.has_id() || ledger_node.has_depth())
        return false;
    return ledger_node.has_nodeid();
}

std::optional<intr_ptr::SharedPtr<SHAMapTreeNode>>
getTreeNode(std::string const& data)
{
    auto const slice = makeSlice(data);
    try
    {
        return SHAMapTreeNode::makeFromWire(slice);
    }
    catch (std::exception const&)
    {
        return std::nullopt;
    }
}

std::optional<SHAMapNodeID>
getSHAMapNodeID(
    Application& app,
    protocol::TMLedgerNode const& ledger_node,
    intr_ptr::SharedPtr<SHAMapTreeNode> const& treeNode)
{
    // When the amendment is enabled and a node depth is present, we can calculate the node ID.
    if (app.getAmendmentTable().isEnabled(fixLedgerNodeID))
    {
        if (treeNode->isInner())
        {
            XRPL_ASSERT(ledger_node.has_id(), "xrpl::getSHAMapNodeID : node ID is present");
            if (!ledger_node.has_id())
                return std::nullopt;

            return deserializeSHAMapNodeID(ledger_node.id());
        }

        if (treeNode->isLeaf())
        {
            XRPL_ASSERT(ledger_node.has_depth(), "xrpl::getSHAMapNodeID : node depth is present");
            if (!ledger_node.has_depth())
                return std::nullopt;

            auto const key = static_cast<SHAMapLeafNode const*>(treeNode.get())->peekItem()->key();
            return SHAMapNodeID::createID(ledger_node.depth(), key);
        }

        UNREACHABLE("xrpl::getSHAMapNodeID : tree node is neither inner nor leaf");
        return std::nullopt;
    }

    // When the amendment is disabled, we expect the node ID to always be present. For leaf nodes
    // we perform an extra check to ensure the node's position in the tree is consistent with its
    // content.
    XRPL_ASSERT(ledger_node.has_nodeid(), "xrpl::getSHAMapNodeID : node ID is present");
    if (!ledger_node.has_nodeid())
        return std::nullopt;

    auto const nodeID = deserializeSHAMapNodeID(ledger_node.nodeid());
    if (!nodeID)
        return std::nullopt;

    if (treeNode->isLeaf())
    {
        auto const key = static_cast<SHAMapLeafNode const*>(treeNode.get())->peekItem()->key();
        auto const expected_id = SHAMapNodeID::createID(static_cast<int>(nodeID->getDepth()), key);
        if (nodeID->getNodeID() != expected_id.getNodeID())
            return std::nullopt;
    }

    return nodeID;
}

}  // namespace xrpl
