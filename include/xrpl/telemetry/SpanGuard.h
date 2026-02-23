#pragma once

#ifdef XRPL_ENABLE_TELEMETRY

#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/context/runtime_context.h>
#include <opentelemetry/nostd/shared_ptr.h>

#include <string_view>
#include <exception>

namespace xrpl {
namespace telemetry {

class SpanGuard
{
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span_;
    opentelemetry::trace::Scope scope_;

public:
    explicit SpanGuard(
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span)
        : span_(std::move(span)), scope_(span_)
    {
    }

    SpanGuard(SpanGuard const&) = delete;
    SpanGuard& operator=(SpanGuard const&) = delete;
    SpanGuard(SpanGuard&& other) noexcept
        : span_(std::move(other.span_)), scope_(span_)
    {
        other.span_ = nullptr;
    }
    SpanGuard& operator=(SpanGuard&&) = delete;

    ~SpanGuard()
    {
        if (span_)
            span_->End();
    }

    opentelemetry::trace::Span&
    span()
    {
        return *span_;
    }

    opentelemetry::trace::Span const&
    span() const
    {
        return *span_;
    }

    void
    setOk()
    {
        span_->SetStatus(opentelemetry::trace::StatusCode::kOk);
    }

    void
    setStatus(
        opentelemetry::trace::StatusCode code,
        std::string_view description = "")
    {
        span_->SetStatus(code, std::string(description));
    }

    template <typename T>
    void
    setAttribute(std::string_view key, T&& value)
    {
        span_->SetAttribute(
            opentelemetry::nostd::string_view(key.data(), key.size()),
            std::forward<T>(value));
    }

    void
    addEvent(std::string_view name)
    {
        span_->AddEvent(std::string(name));
    }

    void
    recordException(std::exception const& e)
    {
        span_->AddEvent("exception", {
            {"exception.type", "std::exception"},
            {"exception.message", std::string(e.what())}
        });
        span_->SetStatus(
            opentelemetry::trace::StatusCode::kError, e.what());
    }

    opentelemetry::context::Context
    context() const
    {
        return opentelemetry::context::RuntimeContext::GetCurrent();
    }
};

}  // namespace telemetry
}  // namespace xrpl

#endif  // XRPL_ENABLE_TELEMETRY
