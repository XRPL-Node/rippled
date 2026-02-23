#include <xrpl/telemetry/Telemetry.h>

#ifdef XRPL_ENABLE_TELEMETRY
#include <opentelemetry/trace/noop.h>
#endif

namespace xrpl {
namespace telemetry {

namespace {

class NullTelemetry : public Telemetry
{
    Setup setup_;

public:
    explicit NullTelemetry(Setup const& setup) : setup_(setup)
    {
    }

    void
    start() override
    {
    }

    void
    stop() override
    {
    }

    bool
    isEnabled() const override
    {
        return false;
    }

    bool
    shouldTraceTransactions() const override
    {
        return false;
    }

    bool
    shouldTraceConsensus() const override
    {
        return false;
    }

    bool
    shouldTraceRpc() const override
    {
        return false;
    }

    bool
    shouldTracePeer() const override
    {
        return false;
    }

#ifdef XRPL_ENABLE_TELEMETRY
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>
    getTracer(std::string_view) override
    {
        static auto noopTracer =
            opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>(
                new opentelemetry::trace::NoopTracer());
        return noopTracer;
    }

    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
    startSpan(std::string_view, opentelemetry::trace::SpanKind) override
    {
        return opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>(
            new opentelemetry::trace::NoopSpan(nullptr));
    }

    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
    startSpan(
        std::string_view,
        opentelemetry::context::Context const&,
        opentelemetry::trace::SpanKind) override
    {
        return opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>(
            new opentelemetry::trace::NoopSpan(nullptr));
    }
#endif
};

}  // namespace

// When XRPL_ENABLE_TELEMETRY is off OR setup.enabled is false,
// return NullTelemetry
#ifndef XRPL_ENABLE_TELEMETRY
std::unique_ptr<Telemetry>
make_Telemetry(Telemetry::Setup const& setup, beast::Journal)
{
    return std::make_unique<NullTelemetry>(setup);
}
#endif

}  // namespace telemetry
}  // namespace xrpl
