#ifndef LUALIKE_ERROR_H_
#define LUALIKE_ERROR_H_

#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "lualike/token.h"

namespace lualike::error {

struct CallFrame {
  std::string function_name;
  std::optional<token::SourceSpan> call_site;

  bool operator==(const CallFrame& rhs) const = default;
};

using CallStack = std::vector<CallFrame>;

class ErrorBase : public std::runtime_error {
  std::exception_ptr exception_ptr_;
  std::unique_ptr<ErrorBase> source_;

 protected:
  std::unique_ptr<ErrorBase> CloneSource() const;
  std::exception_ptr GetStoredExceptionPtr() const noexcept;

 public:
  explicit ErrorBase(std::string message,
                     std::unique_ptr<ErrorBase> source = nullptr,
                     std::exception_ptr exception_ptr = {});

  ErrorBase(const ErrorBase&) = delete;
  ErrorBase& operator=(const ErrorBase&) = delete;
  ErrorBase(ErrorBase&&) noexcept = default;
  ErrorBase& operator=(ErrorBase&&) noexcept = default;
  ~ErrorBase() override = default;

  virtual std::unique_ptr<ErrorBase> Clone() const = 0;
  virtual std::optional<token::SourceSpan> GetContextSpan() const noexcept;
  std::optional<std::string_view> GetContext(
      std::string_view source_text) const noexcept;
  virtual const CallStack* GetCallStack() const noexcept;

  const ErrorBase* Source() const noexcept;
  std::exception_ptr GetExceptionPtr() const noexcept;
};

class ContextError final : public ErrorBase {
  std::optional<token::SourceSpan> context_span_;
  std::optional<CallStack> call_stack_;

 public:
  explicit ContextError(std::string message,
                        std::unique_ptr<ErrorBase> source = nullptr,
                        std::optional<token::SourceSpan> context_span =
                            std::nullopt,
                        std::optional<CallStack> call_stack = std::nullopt,
                        std::exception_ptr exception_ptr = {});

  std::unique_ptr<ErrorBase> Clone() const override;
  std::optional<token::SourceSpan> GetContextSpan() const noexcept override;
  const CallStack* GetCallStack() const noexcept override;
};

class Error : public std::exception {
  std::unique_ptr<ErrorBase> root_;
  std::string source_text_;
  mutable std::string cached_what_;

  explicit Error(std::unique_ptr<ErrorBase> root, std::string source_text);

  std::string BuildWhat() const;

 public:
  explicit Error(std::unique_ptr<ErrorBase> root);

  Error(const Error& rhs);
  Error& operator=(const Error& rhs);
  Error(Error&&) noexcept = default;
  Error& operator=(Error&&) noexcept = default;
  ~Error() override = default;

  static Error Message(std::string message);
  static Error Context(std::string message,
                       std::optional<token::SourceSpan> context_span =
                           std::nullopt,
                       std::optional<CallStack> call_stack = std::nullopt);
  static Error FromException(std::string message, std::exception_ptr exception,
                             std::optional<token::SourceSpan> context_span =
                                 std::nullopt,
                             std::optional<CallStack> call_stack =
                                 std::nullopt);
  static Error FromCurrentException(
      std::string message,
      std::optional<token::SourceSpan> context_span = std::nullopt,
      std::optional<CallStack> call_stack = std::nullopt);

  Error&& Wrap(std::string message) &&;
  Error&& Wrap(std::string message, token::SourceSpan context_span) &&;
  Error&& Wrap(std::string message, CallStack call_stack) &&;
  Error&& Wrap(std::string message,
               std::optional<token::SourceSpan> context_span,
               std::optional<CallStack> call_stack) &&;
  Error&& AttachSourceText(std::string source_text) &&;

  const ErrorBase* Root() const noexcept;
  const std::string& SourceText() const noexcept;
  bool HasSourceText() const noexcept;

  const char* what() const noexcept override;
  std::vector<std::string_view> Messages() const;
  std::string RenderPlain() const;
  std::string RenderPretty() const;
  std::string RenderPretty(std::string_view source_text) const;
};

}  // namespace lualike::error

#endif  // LUALIKE_ERROR_H_
