#pragma once

#include <xrpl/tx/transactors/smart_escrow/HostFunc.h>

#include <string_view>

namespace xrpl {

// Generic WASM constants
static std::string_view const W_ENV = "env";
static std::string_view const W_HOST_LIB = "host_lib";
static std::string_view const W_MEM = "memory";
static std::string_view const W_STORE = "store";
static std::string_view const W_LOAD = "load";
static std::string_view const W_SIZE = "size";
static std::string_view const W_ALLOC = "allocate";
static std::string_view const W_DEALLOC = "deallocate";
static std::string_view const W_PROC_EXIT = "proc_exit";

uint32_t const MAX_PAGES = 128;  // 8MB = 64KB*128

class WasmiRuntime;

/**
 * WasmEngine - Singleton facade for WASM runtime execution.
 *
 * This class provides a high-level interface to the underlying Wasmi runtime,
 * hiding the implementation details and providing a simple API for WASM execution.
 */
class WasmEngine
{
    std::unique_ptr<WasmiRuntime> const impl;

    WasmEngine();

    WasmEngine(WasmEngine const&) = delete;
    WasmEngine(WasmEngine&&) = delete;
    WasmEngine&
    operator=(WasmEngine const&) = delete;
    WasmEngine&
    operator=(WasmEngine&&) = delete;

public:
    ~WasmEngine() = default;

    static WasmEngine&
    instance();

    Expected<WasmResult<int32_t>, TER>
    run(Bytes const& wasmCode,
        std::string_view funcName = {},
        std::vector<WasmParam> const& params = {},
        std::shared_ptr<ImportVec> const& imports = {},
        std::shared_ptr<HostFunctions> const& hfs = {},
        int64_t gasLimit = -1,
        beast::Journal j = beast::Journal{beast::Journal::getNullSink()});

    NotTEC
    check(
        Bytes const& wasmCode,
        std::string_view funcName,
        std::vector<WasmParam> const& params = {},
        std::shared_ptr<ImportVec> const& imports = {},
        std::shared_ptr<HostFunctions> const& hfs = {},
        beast::Journal j = beast::Journal{beast::Journal::getNullSink()});

    // Host functions helper functionality
    void*
    newTrap(std::string const& txt = std::string());

    beast::Journal
    getJournal() const;
};

}  // namespace xrpl
