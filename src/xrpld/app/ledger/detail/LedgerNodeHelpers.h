#pragma once

#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/AmendmentTable.h>

#include <xrpl/basics/IntrusivePointer.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/shamap/SHAMapLeafNode.h>
#include <xrpl/shamap/SHAMapNodeID.h>
#include <xrpl/shamap/SHAMapTreeNode.h>

namespace xrpl {

inline bool
validateLedgerNode(Application& app, protocol::TMLedgerNode const& ledger_node)
{
    if (!ledger_node.has_nodedata())
        return false;

    // When the amendment is enabled, we expect the node ID to be present in the ledger node for
    // inner nodes, and the node depth to be present for leaf nodes. As we cannot confirm whether
    // the node is actually an inner or leaf node here, we will need to perform additional checks
    // separately.
    if (app.getAmendmentTable().isEnabled(fixLedgerNodeID))
        return ledger_node.has_id() || ledger_node.has_depth();

    // When the amendment is disabled, we expect the node ID to always be present.
    return ledger_node.has_nodeid();
}

inline std::optional<intr_ptr::SharedPtr<SHAMapTreeNode>>
getTreeNode(std::string const& data)
{
    auto const slice = makeSlice(data);
    try
    {
        return SHAMapTreeNode::makeFromWire(slice);
    }
    catch (std::exception const&)
    {
        // We can use expected instead once we support C++23.
        return std::nullopt;
    }
}

inline std::optional<SHAMapNodeID>
getSHAMapNodeID(
    Application& app,
    protocol::TMLedgerNode const& ledger_node,
    intr_ptr::SharedPtr<SHAMapTreeNode> const& tree_node)
{
    // When the amendment is enabled and a node depth is present, we can calculate the node ID.
    if (app.getAmendmentTable().isEnabled(fixLedgerNodeID))
    {
        if (tree_node->isInner())
        {
            XRPL_ASSERT(ledger_node.has_id(), "xrpl::getSHAMapNodeID : node ID is present");
            if (!ledger_node.has_id())
                return std::nullopt;

            return deserializeSHAMapNodeID(ledger_node.id());
        }

        if (tree_node->isLeaf())
        {
            XRPL_ASSERT(ledger_node.has_depth(), "xrpl::getSHAMapNodeID : node depth is present");
            if (!ledger_node.has_depth())
                return std::nullopt;

            auto const key = static_cast<SHAMapLeafNode const*>(tree_node.get())->peekItem()->key();
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

    auto const node_id = deserializeSHAMapNodeID(ledger_node.nodeid());
    if (!node_id)
        return std::nullopt;

    if (tree_node->isLeaf())
    {
        auto const key = static_cast<SHAMapLeafNode const*>(tree_node.get())->peekItem()->key();
        auto const expected_id = SHAMapNodeID::createID(static_cast<int>(node_id->getDepth()), key);
        if (node_id->getNodeID() != expected_id.getNodeID())
            return std::nullopt;
    }

    return node_id;
}

}  // namespace xrpl
