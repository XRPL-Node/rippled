#include <xrpl/tx/transactors/smart_escrow/wasm/WasmEngine.h>
#include <xrpl/tx/transactors/smart_escrow/wasm/WasmiRuntime.h>

#include <memory>

namespace xrpl {

WasmEngine::WasmEngine() : impl(std::make_unique<WasmiRuntime>())
{
}

WasmEngine&
WasmEngine::instance()
{
    static WasmEngine e;
    return e;
}

Expected<WasmResult<int32_t>, TER>
WasmEngine::run(
    Bytes const& wasmCode,
    std::string_view funcName,
    std::vector<WasmParam> const& params,
    std::shared_ptr<ImportVec> const& imports,
    std::shared_ptr<HostFunctions> const& hfs,
    int64_t gasLimit,
    beast::Journal j)
{
    return impl->run(wasmCode, funcName, params, imports, hfs, gasLimit, j);
}

NotTEC
WasmEngine::check(
    Bytes const& wasmCode,
    std::string_view funcName,
    std::vector<WasmParam> const& params,
    std::shared_ptr<ImportVec> const& imports,
    std::shared_ptr<HostFunctions> const& hfs,
    beast::Journal j)
{
    return impl->check(wasmCode, funcName, params, imports, hfs, j);
}

void*
WasmEngine::newTrap(std::string const& msg)
{
    return impl->newTrap(msg);
}

// LCOV_EXCL_START
beast::Journal
WasmEngine::getJournal() const
{
    return impl->getJournal();
}
// LCOV_EXCL_STOP

}  // namespace xrpl
