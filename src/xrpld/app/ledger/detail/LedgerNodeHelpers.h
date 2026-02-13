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
    // inner nodes, while we expect it to not be present in the ledger node for leaf nodes. However,
    // at this point we don't yet know whether the node is an inner node or a leaf node, so we
    // allow both cases.
    if (app.getAmendmentTable().isEnabled(fixLedgerNodeID))
        return true;

    // When the amendment is disabled, we expect the node ID to always be present.
    return ledger_node.has_nodeid();
}

inline std::optional<intr_ptr::SharedPtr<SHAMapTreeNode>>
getTreeNode(Slice const& node_slice)
{
    try
    {
        return SHAMapTreeNode::makeFromWire(node_slice);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

inline std::optional<SHAMapNodeID>
getSHAMapNodeID(
    Application& app,
    protocol::TMLedgerNode const& ledger_node,
    intr_ptr::SharedPtr<SHAMapTreeNode> const& tree_node)
{
    // When the amendment is enabled, we can get the node ID directly from the ledger node for inner
    // nodes, while we compute it for leaf nodes.
    if (app.getAmendmentTable().isEnabled(fixLedgerNodeID))
    {
        if (tree_node->isInner())
        {
            XRPL_ASSERT(ledger_node.has_nodeid(), "xrpl::getSHAMapNodeID : node ID is present");
            if (!ledger_node.has_nodeid())
                return std::nullopt;

            return deserializeSHAMapNodeID(ledger_node.nodeid());
        }

        if (tree_node->isLeaf())
        {
            XRPL_ASSERT(!ledger_node.has_nodeid(), "xrpl::getSHAMapNodeID : node ID is not present");
            if (ledger_node.has_nodeid())
                return std::nullopt;

            auto const key = static_cast<SHAMapLeafNode const*>(tree_node.get())->peekItem()->key();
            return SHAMapNodeID::createID(SHAMap::leafDepth, key);
        }

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
