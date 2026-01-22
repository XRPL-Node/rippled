#ifndef XRPL_PROTOCOL_NFTSYNTHETICSERIALIZER_H_INCLUDED
#define XRPL_PROTOCOL_NFTSYNTHETICSERIALIZER_H_INCLUDED

#include <xrpl/json/json_forwards.h>

#include <memory>

namespace xrpl {

class STTx;
class TxMeta;

namespace RPC {

/**
   Adds common synthetic fields to transaction-related JSON responses

   @{
 */
void
insertNFTSyntheticInJson(
    Json::Value&,
    std::shared_ptr<STTx const> const&,
    TxMeta const&);
/** @} */

}  // namespace RPC
}  // namespace xrpl

#endif
