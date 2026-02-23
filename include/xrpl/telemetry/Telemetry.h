#pragma once

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/basics/BasicConfig.h>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#ifdef XRPL_ENABLE_TELEMETRY
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/context/context.h>
#include <opentelemetry/nostd/shared_ptr.h>
#endif

namespace xrpl {
namespace telemetry {

class Telemetry
{
public:
    struct Setup
    {
        bool enabled = false;
        std::string serviceName = "rippled";
        std::string serviceVersion;
        std::string serviceInstanceId;

        std::string exporterType = "otlp_http";
        std::string exporterEndpoint = "http://localhost:4318/v1/traces";
        bool useTls = false;
        std::string tlsCertPath;

        double samplingRatio = 1.0;

        std::uint32_t batchSize = 512;
        std::chrono::milliseconds batchDelay{5000};
        std::uint32_t maxQueueSize = 2048;

        std::uint32_t networkId = 0;
        std::string networkType = "mainnet";

        bool traceTransactions = true;
        bool traceConsensus = true;
        bool traceRpc = true;
        bool tracePeer = false;
        bool traceLedger = true;
    };

    virtual ~Telemetry() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isEnabled() const = 0;

    virtual bool shouldTraceTransactions() const = 0;
    virtual bool shouldTraceConsensus() const = 0;
    virtual bool shouldTraceRpc() const = 0;
    virtual bool shouldTracePeer() const = 0;

#ifdef XRPL_ENABLE_TELEMETRY
    virtual opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>
    getTracer(std::string_view name = "rippled") = 0;

    virtual opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
    startSpan(
        std::string_view name,
        opentelemetry::trace::SpanKind kind =
            opentelemetry::trace::SpanKind::kInternal) = 0;

    virtual opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
    startSpan(
        std::string_view name,
        opentelemetry::context::Context const& parentContext,
        opentelemetry::trace::SpanKind kind =
            opentelemetry::trace::SpanKind::kInternal) = 0;
#endif
};

std::unique_ptr<Telemetry>
make_Telemetry(Telemetry::Setup const& setup, beast::Journal journal);

Telemetry::Setup
setup_Telemetry(
    Section const& section,
    std::string const& nodePublicKey,
    std::string const& version);

}  // namespace telemetry
}  // namespace xrpl
