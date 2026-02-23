#pragma once

#ifdef XRPL_ENABLE_TELEMETRY

#include <xrpl/telemetry/Telemetry.h>
#include <xrpl/telemetry/SpanGuard.h>

#include <optional>

namespace xrpl {
namespace telemetry {

// Start a span that is automatically ended when guard goes out of scope
#define XRPL_TRACE_SPAN(_tel_obj_, _span_name_) \
    auto _xrpl_span_ = (_tel_obj_).startSpan(_span_name_); \
    ::xrpl::telemetry::SpanGuard _xrpl_guard_(_xrpl_span_)

// Start a span with specific kind
#define XRPL_TRACE_SPAN_KIND(_tel_obj_, _span_name_, _span_kind_) \
    auto _xrpl_span_ = (_tel_obj_).startSpan(_span_name_, _span_kind_); \
    ::xrpl::telemetry::SpanGuard _xrpl_guard_(_xrpl_span_)

// Conditional span for RPC tracing
#define XRPL_TRACE_RPC(_tel_obj_, _span_name_) \
    std::optional<::xrpl::telemetry::SpanGuard> _xrpl_guard_; \
    if ((_tel_obj_).shouldTraceRpc()) { \
        _xrpl_guard_.emplace((_tel_obj_).startSpan(_span_name_)); \
    }

// Conditional span for transaction tracing
#define XRPL_TRACE_TX(_tel_obj_, _span_name_) \
    std::optional<::xrpl::telemetry::SpanGuard> _xrpl_guard_; \
    if ((_tel_obj_).shouldTraceTransactions()) { \
        _xrpl_guard_.emplace((_tel_obj_).startSpan(_span_name_)); \
    }

// Conditional span for consensus tracing
#define XRPL_TRACE_CONSENSUS(_tel_obj_, _span_name_) \
    std::optional<::xrpl::telemetry::SpanGuard> _xrpl_guard_; \
    if ((_tel_obj_).shouldTraceConsensus()) { \
        _xrpl_guard_.emplace((_tel_obj_).startSpan(_span_name_)); \
    }

// Set attribute on current span (if exists)
#define XRPL_TRACE_SET_ATTR(key, value) \
    if (_xrpl_guard_.has_value()) { \
        _xrpl_guard_->setAttribute(key, value); \
    }

// Record exception on current span
#define XRPL_TRACE_EXCEPTION(e) \
    if (_xrpl_guard_.has_value()) { \
        _xrpl_guard_->recordException(e); \
    }

}  // namespace telemetry
}  // namespace xrpl

#else  // XRPL_ENABLE_TELEMETRY not defined

#define XRPL_TRACE_SPAN(_tel_obj_, _span_name_) ((void)0)
#define XRPL_TRACE_SPAN_KIND(_tel_obj_, _span_name_, _span_kind_) ((void)0)
#define XRPL_TRACE_RPC(_tel_obj_, _span_name_) ((void)0)
#define XRPL_TRACE_TX(_tel_obj_, _span_name_) ((void)0)
#define XRPL_TRACE_CONSENSUS(_tel_obj_, _span_name_) ((void)0)
#define XRPL_TRACE_SET_ATTR(key, value) ((void)0)
#define XRPL_TRACE_EXCEPTION(e) ((void)0)

#endif  // XRPL_ENABLE_TELEMETRY
