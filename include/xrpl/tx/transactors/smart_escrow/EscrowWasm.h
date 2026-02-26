#pragma once

#include <xrpl/tx/transactors/smart_escrow/HostFunc.h>

#include <string_view>
#include <vector>

namespace xrpl {

// Escrow-specific WASM constants
static std::string_view const ESCROW_FUNCTION_NAME = "finish";

/**
 * Creates WASM imports for smart escrow execution.
 *
 * Binds all host functions (ledger access, crypto, float operations, etc.)
 * to the WASM import table with their associated gas costs.
 *
 * @param hfs The host functions implementation to bind
 * @return Shared pointer to the import vector
 */
std::shared_ptr<ImportVec>
createWasmImport(HostFunctions& hfs);

/**
 * Runs smart escrow WASM code.
 *
 * Executes the specified function in the WASM module with the given parameters
 * and gas limit.
 *
 * @param wasmCode The compiled WASM bytecode
 * @param hfs Host functions implementation
 * @param funcName Function to execute (default: "finish")
 * @param params Parameters to pass to the function
 * @param gasLimit Maximum gas to consume (-1 for unlimited)
 * @return EscrowResult on success, TER error code on failure
 */
Expected<EscrowResult, TER>
runEscrowWasm(
    Bytes const& wasmCode,
    std::shared_ptr<HostFunctions> const& hfs,
    std::string_view funcName = ESCROW_FUNCTION_NAME,
    std::vector<WasmParam> const& params = {},
    int64_t gasLimit = -1);

/**
 * Validates smart escrow WASM code during preflight.
 *
 * Checks that the WASM module is valid and contains the expected function
 * signature without actually executing the code.
 *
 * @param wasmCode The compiled WASM bytecode
 * @param hfs Host functions implementation
 * @param funcName Function to validate (default: "finish")
 * @param params Parameters to validate against
 * @return tesSUCCESS if valid, error code otherwise
 */
NotTEC
preflightEscrowWasm(
    Bytes const& wasmCode,
    std::shared_ptr<HostFunctions> const& hfs,
    std::string_view funcName = ESCROW_FUNCTION_NAME,
    std::vector<WasmParam> const& params = {});

}  // namespace xrpl
