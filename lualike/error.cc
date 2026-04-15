#include "lualike/error.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ostream>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>

namespace lualike::error {

namespace {

const ErrorBase* LastErrorInChain(const ErrorBase* err) {
  const ErrorBase* current = err;
  while (current != nullptr && current->Source() != nullptr) {
    current = current->Source();
  }

  return current;
}

std::string DescribeException(std::exception_ptr exception_ptr) {
  if (!exception_ptr) {
    return {};
  }

  try {
    std::rethrow_exception(exception_ptr);
  } catch (const Error& err) {
    return err.RenderPlain();
  } catch (const std::exception& err) {
    return err.what();
  } catch (...) {
    return "unknown foreign exception";
  }
}

std::vector<const ErrorBase*> CollectChain(const ErrorBase* root) {
  std::vector<const ErrorBase*> chain;
  for (auto* current = root; current != nullptr; current = current->Source()) {
    chain.push_back(current);
  }

  return chain;
}

struct LineInfo {
  size_t line_number{};
  size_t line_begin{};
  size_t line_end{};
};

LineInfo LocateLine(std::string_view source_text, size_t offset) {
  offset = std::min(offset, source_text.size());

  LineInfo line{1, 0, source_text.size()};
  for (size_t index = 0; index < offset; ++index) {
    if (source_text[index] == '\n') {
      ++line.line_number;
      line.line_begin = index + 1;
    }
  }

  if (const size_t line_end = source_text.find('\n', line.line_begin);
      line_end != std::string_view::npos) {
    line.line_end = line_end;
  }

  return line;
}

std::string RenderSpanSnippet(std::string_view source_text,
                              token::SourceSpan span,
                              std::string_view message) {
  if (source_text.empty()) {
    return {};
  }

  span.begin = std::min(span.begin, source_text.size());
  span.end = std::clamp(span.end, span.begin, source_text.size());

  const auto line = LocateLine(source_text, span.begin);
  const auto line_text =
      source_text.substr(line.line_begin, line.line_end - line.line_begin);
  const size_t column_begin = span.begin - line.line_begin;
  const size_t column_end = std::min(
      std::max(span.end, span.begin + static_cast<size_t>(1)), line.line_end);
  const size_t caret_count =
      std::max(column_end - line.line_begin - column_begin, static_cast<size_t>(1));
  const auto line_number_text = std::to_string(line.line_number);

  std::ostringstream out;
  out << line_number_text << " | " << line_text << '\n';
  out << std::string(line_number_text.size(), ' ') << " | "
      << std::string(column_begin, ' ') << std::string(caret_count, '^') << ' '
      << message;
  return out.str();
}

void AppendCallStack(std::ostringstream* out, const CallStack& call_stack) {
  if (call_stack.empty()) {
    return;
  }

  *out << "\ncall stack:";
  for (const auto& frame : std::views::reverse(call_stack)) {
    *out << "\n  at " << frame.function_name;
    if (frame.call_site) {
      *out << " [" << frame.call_site->begin << ", " << frame.call_site->end
           << ')';
    }
  }
}

}  // namespace

ErrorBase::ErrorBase(std::string message, std::unique_ptr<ErrorBase> source,
                     std::exception_ptr exception_ptr)
    : std::runtime_error(std::move(message)),
      exception_ptr_(std::move(exception_ptr)),
      source_(std::move(source)) {}

std::unique_ptr<ErrorBase> ErrorBase::CloneSource() const {
  return source_ ? source_->Clone() : nullptr;
}

std::exception_ptr ErrorBase::GetStoredExceptionPtr() const noexcept {
  return exception_ptr_;
}

std::optional<token::SourceSpan> ErrorBase::GetContextSpan() const noexcept {
  return std::nullopt;
}

std::optional<std::string_view> ErrorBase::GetContext(
    std::string_view source_text) const noexcept {
  const auto span = GetContextSpan();
  if (!span) {
    return std::nullopt;
  }

  if (span->begin > source_text.size() || span->end > source_text.size() ||
      span->begin > span->end) {
    return std::nullopt;
  }

  return source_text.substr(span->begin, span->end - span->begin);
}

const CallStack* ErrorBase::GetCallStack() const noexcept { return nullptr; }

const ErrorBase* ErrorBase::Source() const noexcept { return source_.get(); }

std::exception_ptr ErrorBase::GetExceptionPtr() const noexcept {
  return exception_ptr_;
}

ContextError::ContextError(std::string message, std::unique_ptr<ErrorBase> source,
                           std::optional<token::SourceSpan> context_span,
                           std::optional<CallStack> call_stack,
                           std::exception_ptr exception_ptr)
    : ErrorBase(std::move(message), std::move(source),
                std::move(exception_ptr)),
      context_span_(context_span),
      call_stack_(std::move(call_stack)) {}

std::unique_ptr<ErrorBase> ContextError::Clone() const {
  return std::make_unique<ContextError>(what(), CloneSource(), context_span_,
                                        call_stack_, GetStoredExceptionPtr());
}

std::optional<token::SourceSpan> ContextError::GetContextSpan() const noexcept {
  return context_span_;
}

const CallStack* ContextError::GetCallStack() const noexcept {
  return call_stack_ ? &call_stack_.value() : nullptr;
}

Error::Error(std::unique_ptr<ErrorBase> root, std::string source_text)
    : root_(std::move(root)), source_text_(std::move(source_text)) {}

Error::Error(std::unique_ptr<ErrorBase> root) : Error(std::move(root), {}) {}

Error::Error(const Error& rhs)
    : root_(rhs.root_ ? rhs.root_->Clone() : nullptr),
      source_text_(rhs.source_text_),
      cached_what_(rhs.cached_what_) {}

Error& Error::operator=(const Error& rhs) {
  if (this == &rhs) {
    return *this;
  }

  root_ = rhs.root_ ? rhs.root_->Clone() : nullptr;
  source_text_ = rhs.source_text_;
  cached_what_ = rhs.cached_what_;
  return *this;
}

Error Error::Message(std::string message) {
  return Error(std::make_unique<ContextError>(std::move(message)));
}

Error Error::Context(std::string message,
                     std::optional<token::SourceSpan> context_span,
                     std::optional<CallStack> call_stack) {
  return Error(std::make_unique<ContextError>(
      std::move(message), nullptr, context_span, std::move(call_stack)));
}

Error Error::FromException(std::string message, std::exception_ptr exception,
                           std::optional<token::SourceSpan> context_span,
                           std::optional<CallStack> call_stack) {
  return Error(std::make_unique<ContextError>(
      std::move(message), nullptr, context_span, std::move(call_stack),
      std::move(exception)));
}

Error Error::FromCurrentException(
    std::string message, std::optional<token::SourceSpan> context_span,
    std::optional<CallStack> call_stack) {
  return FromException(std::move(message), std::current_exception(),
                       context_span, std::move(call_stack));
}

Error&& Error::Wrap(std::string message) && {
  root_ = std::make_unique<ContextError>(std::move(message), std::move(root_));
  cached_what_.clear();
  return std::move(*this);
}

Error&& Error::Wrap(std::string message, token::SourceSpan context_span) && {
  return std::move(*this).Wrap(std::move(message), context_span, std::nullopt);
}

Error&& Error::Wrap(std::string message, CallStack call_stack) && {
  return std::move(*this).Wrap(std::move(message), std::nullopt,
                               std::move(call_stack));
}

Error&& Error::Wrap(std::string message,
                    std::optional<token::SourceSpan> context_span,
                    std::optional<CallStack> call_stack) && {
  root_ = std::make_unique<ContextError>(std::move(message), std::move(root_),
                                         context_span, std::move(call_stack));
  cached_what_.clear();
  return std::move(*this);
}

Error&& Error::AttachSourceText(std::string source_text) && {
  if (source_text_.empty()) {
    source_text_ = std::move(source_text);
  }
  return std::move(*this);
}

const ErrorBase* Error::Root() const noexcept { return root_.get(); }

const std::string& Error::SourceText() const noexcept { return source_text_; }

bool Error::HasSourceText() const noexcept { return !source_text_.empty(); }

std::string Error::BuildWhat() const {
  std::ostringstream out;
  bool is_first_message = true;
  for (const auto* current = root_.get(); current != nullptr;
       current = current->Source()) {
    if (!is_first_message) {
      out << ": ";
    }

    out << current->what();
    is_first_message = false;
  }

  if (const auto* leaf = LastErrorInChain(root_.get());
      leaf != nullptr && leaf->GetExceptionPtr()) {
    const auto foreign_exception_text =
        DescribeException(leaf->GetExceptionPtr());
    if (!foreign_exception_text.empty()) {
      if (!is_first_message) {
        out << ": ";
      }
      out << foreign_exception_text;
    }
  }

  return out.str();
}

const char* Error::what() const noexcept {
  try {
    if (cached_what_.empty()) {
      cached_what_ = BuildWhat();
    }
    return cached_what_.c_str();
  } catch (...) {
    return root_ != nullptr ? root_->what() : "unknown error";
  }
}

std::vector<std::string_view> Error::Messages() const {
  std::vector<std::string_view> messages;
  for (const auto* current = root_.get(); current != nullptr;
       current = current->Source()) {
    messages.push_back(current->what());
  }

  return messages;
}

std::string Error::RenderPlain() const {
  std::ostringstream out;
  bool is_first = true;
  for (const auto* current = root_.get(); current != nullptr;
       current = current->Source()) {
    if (!is_first) {
      out << "\ncaused by: ";
    }

    out << current->what();
    if (const auto span = current->GetContextSpan()) {
      out << " [" << span->begin << ", " << span->end << ')';
    }
    if (const auto* call_stack = current->GetCallStack()) {
      AppendCallStack(&out, *call_stack);
    }
    is_first = false;
  }

  if (const auto* leaf = LastErrorInChain(root_.get());
      leaf != nullptr && leaf->GetExceptionPtr()) {
    const auto foreign_exception_text =
        DescribeException(leaf->GetExceptionPtr());
    if (!foreign_exception_text.empty()) {
      out << "\nforeign exception: " << foreign_exception_text;
    }
  }

  return out.str();
}

std::string Error::RenderPretty() const {
  return RenderPretty(source_text_);
}

std::string Error::RenderPretty(std::string_view source_text) const {
  if (source_text.empty()) {
    return RenderPlain();
  }

  const auto chain = CollectChain(root_.get());
  std::ostringstream out;

  bool wrote_source_section = false;
  for (const auto* current : chain) {
    const auto span = current->GetContextSpan();
    if (!span) {
      continue;
    }

    if (!wrote_source_section) {
      out << "Source context:";
      wrote_source_section = true;
    }

    out << "\n\n" << RenderSpanSnippet(source_text, *span, current->what());
    if (const auto* call_stack = current->GetCallStack()) {
      AppendCallStack(&out, *call_stack);
    }
  }

  if (wrote_source_section) {
    out << "\n\n";
  }

  out << "Error chain:";
  for (const auto* current : chain) {
    if (!current->GetContextSpan()) {
      out << "\n- " << current->what();
      if (const auto* call_stack = current->GetCallStack()) {
        AppendCallStack(&out, *call_stack);
      }
    }
  }

  if (const auto* leaf = LastErrorInChain(root_.get());
      leaf != nullptr && leaf->GetExceptionPtr()) {
    const auto foreign_exception_text =
        DescribeException(leaf->GetExceptionPtr());
    if (!foreign_exception_text.empty()) {
      out << "\n- foreign exception: " << foreign_exception_text;
    }
  }

  return out.str();
}

}  // namespace lualike::error
