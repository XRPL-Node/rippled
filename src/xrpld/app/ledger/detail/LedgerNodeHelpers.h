#pragma once

#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/AmendmentTable.h>

#include <xrpl/basics/IntrusivePointer.h>
#include <xrpl/shamap/SHAMapLeafNode.h>
#include <xrpl/shamap/SHAMapNodeID.h>
#include <xrpl/shamap/SHAMapTreeNode.h>

namespace xrpl {

inline bool
validateLedgerNode(Application& app, protocol::TMLedgerNode const& ledger_node)
{
    if (!ledger_node.has_nodedata())
        return false;

    if (app.getAmendmentTable().isEnabled(fixLedgerNodeDepth))
        return ledger_node.has_nodedepth() && ledger_node.nodedepth() < 65;

    return ledger_node.has_nodeid();
}

inline std::optional<SHAMapNodeID>
getSHAMapNodeID(
    Application& app,
    protocol::TMLedgerNode const& ledger_node,
    intr_ptr::SharedPtr<SHAMapTreeNode> const& tree_node)
{
    SHAMapNodeID node_id;
    if (app.getAmendmentTable().isEnabled(fixLedgerNodeDepth))
    {
        node_id = SHAMapNodeID(ledger_node.nodedepth(), tree_node->getHash().as_uint256());
    }
    else
    {
        auto const nid = deserializeSHAMapNodeID(ledger_node.nodeid());
        if (!nid)
            return std::nullopt;
        node_id = *nid;
    }

    // For leaf nodes, verify that the node ID is actually the same as what the node ID should be,
    // given the position of the node in the SHAMap.
    if (tree_node->isLeaf())
    {
        auto const nodeKey = dynamic_cast<SHAMapLeafNode const*>(tree_node.get())->peekItem()->key();
        auto const expectedID = SHAMapNodeID::createID(static_cast<int>(node_id.getDepth()), nodeKey);
        if (node_id.getNodeID() != expectedID.getNodeID())
            return std::nullopt;
    }

    return node_id;
}

}  // namespace xrpl
