#pragma once

#include <xrpld/app/ledger/detail/LedgerNodeHelpers.h>
#include <xrpld/app/main/Application.h>

#include <xrpl/basics/IntrusivePointer.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/ledger/AmendmentTable.h>
#include <xrpl/shamap/SHAMapLeafNode.h>
#include <xrpl/shamap/SHAMapNodeID.h>
#include <xrpl/shamap/SHAMapTreeNode.h>

namespace xrpl {

/**
 * @brief Validates a ledger node based on the fixLedgerNodeID amendment status.
 *
 * This function checks whether a ledger node has the expected fields based on
 * whether the fixLedgerNodeID amendment is enabled. The validation rules differ
 * depending on the amendment state:
 *
 * When the amendment is enabled:
 * - The node must have `nodedata`.
 * - The legacy `nodeid` field must NOT be present.
 * - For inner nodes: the `id` field must be present.
 * - For leaf nodes: the `depth` field must be present and <= SHAMap::leafDepth.
 *
 * When the amendment is disabled:
 * - The node must have `nodedata`.
 * - The `nodeid` field must be present.
 * - The new `id` and `depth` fields must NOT be present.
 *
 * @param app The application instance used to check amendment status.
 * @param ledger_node The ledger node to validate.
 * @return true if the ledger node has the expected fields, false otherwise.
 */
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

/**
 * @brief Deserializes a SHAMapTreeNode from wire format data.
 *
 * This function attempts to create a SHAMapTreeNode from the provided data
 * string. If the data is malformed or deserialization fails, the function
 * returns std::nullopt instead of throwing an exception.
 *
 * @param data The serialized node data in wire format.
 * @return An optional containing the deserialized tree node if successful, or
 *         std::nullopt if deserialization fails.
 */
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

/**
 * @brief Extracts or reconstructs the SHAMapNodeID from a ledger node.
 *
 * This function retrieves the SHAMapNodeID for a tree node, with behavior that
 * depends on the fixLedgerNodeID amendment status and the node type (inner vs.
 * leaf).
 *
 * When the fixLedgerNodeID amendment is enabled:
 * - For inner nodes: Deserializes the node ID from the `ledger_node.id` field.
 *   Note that root nodes are also inner nodes.
 * - For leaf nodes: Reconstructs the node ID using both the depth from the
 *   `ledger_node.depth` field and the key from the leaf node's item.
 *
 * When the amendment is disabled:
 * - For all nodes: Deserializes the node ID from the `ledger_node.nodeid`
 *   field.
 * - For leaf nodes: Validates that the node ID is consistent with the leaf's
 *   key.
 *
 * @param app The application instance used to check amendment status.
 * @param ledger_node The protocol message containing the ledger node data.
 * @param treeNode The deserialized tree node (inner or leaf node).
 * @return An optional containing the node ID if extraction/reconstruction
 *         succeeds, or std::nullopt if the required fields are missing or
 *         validation fails.
 */
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
