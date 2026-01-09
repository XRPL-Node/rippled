#ifndef XRPL_PROTOCOL_NFTSYNTHETICSERIALIZER_H_INCLUDED
#define XRPL_PROTOCOL_NFTSYNTHETICSERIALIZER_H_INCLUDED

#include <xrpl/json/json_forwards.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/TxMeta.h>

#include <memory>

namespace xrpl {

namespace RPC {

/**
   Adds common synthetic fields to transaction metadata JSON

   @{
 */
void
insertNFTSyntheticInJson(
    Json::Value& metadata,
    std::shared_ptr<STTx const> const&,
    TxMeta const&);
/** @} */

}  // namespace RPC
}  // namespace xrpl

#endif
