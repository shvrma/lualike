#ifndef LUALIKE_INTERPRETER_H_
#define LUALIKE_INTERPRETER_H_

#include <cstdint>
#include <expected>
#include <format>
#include <istream>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

#include "lualike/ast.h"
#include "lualike/error.h"
#include "lualike/parser.h"
#include "lualike/value.h"

namespace lualike::interpreter {

class Scope {
  using NamesT = std::unordered_map<std::string, value::LualikeValue>;

  NamesT names_;
  std::optional<std::shared_ptr<Scope>> parent_scope_;

 public:
  explicit Scope(std::shared_ptr<Scope> parent_scope)
      : parent_scope_(std::move(parent_scope)) {}

  explicit Scope() = default;

  std::optional<value::LualikeValue> Get(const std::string& name) const {
    if (const auto it = names_.find(name); it != names_.end()) {
      return it->second;
    }

    if (parent_scope_) {
      return parent_scope_.value()->Get(name);
    }

    return std::nullopt;
  }

  void Set(const std::string& name, const value::LualikeValue& value) {
    if (names_.find(name) != names_.end()) {
      names_[name] = value;
      return;
    }

    if (parent_scope_ && parent_scope_.value()->Get(name).has_value()) {
      parent_scope_.value()->Set(name, value);
      return;
    }

    names_[name] = value;
  }
};

inline error::Error MakeInterpreterError(
    std::string message,
    std::optional<token::SourceSpan> context_span = std::nullopt,
    std::optional<error::CallStack> call_stack = std::nullopt) {
  return error::Error::Context(std::move(message), context_span,
                               std::move(call_stack));
}

template <typename ResultT, typename CallbackT>
ResultT ExecuteWithForeignExceptionContext(const std::string& message,
                                           token::SourceSpan context_span,
                                           CallbackT&& callback) {
  try {
    return std::forward<CallbackT>(callback)();
  } catch (const error::Error&) {
    throw;
  } catch (...) {
    throw error::Error::FromCurrentException(message, context_span);
  }
}

template <typename ExprT>
value::LualikeValue ExprVisitor(const ExprT& expr, token::SourceSpan span,
                                std::shared_ptr<Scope> scope);
template <typename StmtT>
std::optional<value::LualikeValue> StmtVisitor(const StmtT& stmt,
                                               token::SourceSpan span,
                                               std::shared_ptr<Scope> scope);

static bool IsTruthy(const value::LualikeValue& value);

value::LualikeValue VisitExpression(const ast::Expression& expression,
                                    std::shared_ptr<Scope> scope);
std::optional<value::LualikeValue> VisitStatement(
    const ast::Statement& statement, std::shared_ptr<Scope> scope);
std::optional<value::LualikeValue> VisitBlock(const ast::Block& block,
                                              std::shared_ptr<Scope> scope);

inline std::expected<std::optional<value::LualikeValue>, error::Error>
Interpret(std::string_view input) noexcept {
  try {
    auto parse_result = parser::detail::ParseSourceView(input);
    if (!parse_result) {
      return std::unexpected(
          std::move(parse_result).error().AttachSourceText(std::string(input)));
    }

    return VisitBlock(parse_result.value(), std::make_shared<Scope>());
  } catch (error::Error& err) {
    return std::unexpected(std::move(err).AttachSourceText(std::string(input)));
  } catch (const std::exception&) {
    return std::unexpected(
        error::Error::FromCurrentException("Internal interpreter error")
            .AttachSourceText(std::string(input)));
  } catch (...) {
    return std::unexpected(error::Error::Message("Unknown interpreter error")
                               .AttachSourceText(std::string(input)));
  }
}

inline std::expected<std::optional<value::LualikeValue>, error::Error>
Interpret(std::istream& input) noexcept {
  auto source_result = parser::detail::ReadStreamToString(input);
  if (!source_result) {
    return std::unexpected(std::move(source_result).error());
  }

  std::string source = std::move(source_result).value();

  try {
    auto parse_result = parser::detail::ParseSourceView(source);
    if (!parse_result) {
      return std::unexpected(
          std::move(parse_result).error().AttachSourceText(std::move(source)));
    }

    return VisitBlock(parse_result.value(), std::make_shared<Scope>());
  } catch (error::Error& err) {
    return std::unexpected(std::move(err).AttachSourceText(std::move(source)));
  } catch (const std::exception&) {
    return std::unexpected(
        error::Error::FromCurrentException("Internal interpreter error")
            .AttachSourceText(std::move(source)));
  } catch (...) {
    return std::unexpected(error::Error::Message("Unknown interpreter error")
                               .AttachSourceText(std::move(source)));
  }
}

inline value::LualikeValue VisitExpression(const ast::Expression& expression,
                                           std::shared_ptr<Scope> scope) {
  return std::visit(
      [&expression, scope](const auto& expr) {
        return ExprVisitor(expr, expression.span, scope);
      },
      expression.node);
}

inline std::optional<value::LualikeValue> VisitStatement(
    const ast::Statement& statement, std::shared_ptr<Scope> scope) {
  return std::visit(
      [&statement, scope](const auto& stmt) {
        return StmtVisitor(stmt, statement.span, scope);
      },
      statement.node);
}

inline std::optional<value::LualikeValue> VisitBlock(
    const ast::Block& block, std::shared_ptr<Scope> scope) {
  const auto inner_scope = std::make_shared<Scope>(scope);
  for (const auto& statement : block.statements) {
    if (auto return_value = VisitStatement(statement, inner_scope)) {
      return return_value;
    }
  }

  return std::nullopt;
}

inline bool IsTruthy(const value::LualikeValue& value) {
  if (std::holds_alternative<value::LualikeValue::NilT>(value.inner_value)) {
    return false;
  }

  if (std::holds_alternative<value::LualikeValue::BoolT>(value.inner_value)) {
    return std::get<value::LualikeValue::BoolT>(value.inner_value);
  }

  return true;
}

template <typename ExprT>
value::LualikeValue ExprVisitor(const ExprT& expr, token::SourceSpan span,
                                std::shared_ptr<Scope> scope) {
  using T = std::decay_t<decltype(expr)>;

  if constexpr (std::is_same_v<T, ast::LiteralExpression>) {
    return expr.value;
  }

  else if constexpr (std::is_same_v<T, ast::VariableExpression>) {
    if (const auto val = scope->Get(expr.name)) {
      return val.value();
    }

    throw MakeInterpreterError(std::format("Unknown variable: '{}'", expr.name),
                               span);
  }

  else if constexpr (std::is_same_v<T, ast::UnaryExpression>) {
    auto rhs = VisitExpression(*expr.rhs, scope);

    switch (expr.op) {
      case ast::UnaryOperator::kNegate:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate unary negation", span, [&rhs] { return -rhs; });
      case ast::UnaryOperator::kNot:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate logical negation", span,
            [&rhs] { return !rhs; });
    }

    throw MakeInterpreterError("Unimplemented unary operator", span);
  }

  else if constexpr (std::is_same_v<T, ast::BinaryExpression>) {
    auto lhs = VisitExpression(*expr.lhs, scope);

    if (expr.op == ast::BinaryOperator::kAnd) {
      return IsTruthy(lhs) ? VisitExpression(*expr.rhs, scope) : lhs;
    }
    if (expr.op == ast::BinaryOperator::kOr) {
      return IsTruthy(lhs) ? lhs : VisitExpression(*expr.rhs, scope);
    }

    const auto rhs = VisitExpression(*expr.rhs, scope);

    switch (expr.op) {
      case ast::BinaryOperator::kAdd:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate addition", span,
            [&lhs, &rhs] { return lhs + rhs; });
      case ast::BinaryOperator::kSubtract:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate subtraction", span,
            [&lhs, &rhs] { return lhs - rhs; });
      case ast::BinaryOperator::kMultiply:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate multiplication", span,
            [&lhs, &rhs] { return lhs * rhs; });
      case ast::BinaryOperator::kDivide:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate division", span,
            [&lhs, &rhs] { return lhs / rhs; });
      case ast::BinaryOperator::kFloorDivide:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate floor division", span, [&lhs, &rhs] {
              lhs.FloorDivideAndAssign(rhs);
              return lhs;
            });
      case ast::BinaryOperator::kModulo:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate modulo", span,
            [&lhs, &rhs] { return lhs % rhs; });
      case ast::BinaryOperator::kPower:
        return ExecuteWithForeignExceptionContext<value::LualikeValue>(
            "Failed to evaluate exponentiation", span, [&lhs, &rhs] {
              lhs.ExponentiateAndAssign(rhs);
              return lhs;
            });
      case ast::BinaryOperator::kEqual:
        return {lhs == rhs};
      case ast::BinaryOperator::kNotEqual:
        return {lhs != rhs};
      case ast::BinaryOperator::kLessThan:
        return {lhs < rhs};
      case ast::BinaryOperator::kLessThanEqual:
        return {lhs <= rhs};
      case ast::BinaryOperator::kGreaterThan:
        return {lhs > rhs};
      case ast::BinaryOperator::kGreaterThanEqual:
        return {lhs >= rhs};
      case ast::BinaryOperator::kAnd:
      case ast::BinaryOperator::kOr:
        break;
    }

    throw MakeInterpreterError("Unimplemented binary operator", span);
  }

  throw MakeInterpreterError("Unimplemented expression type", span);
}

template <typename StmtT>
std::optional<value::LualikeValue> StmtVisitor(const StmtT& stmt,
                                               token::SourceSpan span,
                                               std::shared_ptr<Scope> scope) {
  using T = std::decay_t<decltype(stmt)>;

  if constexpr (std::is_same_v<T, ast::VariableDeclaration>) {
    value::LualikeValue value;
    if (stmt.initializer) {
      value = VisitExpression(stmt.initializer.value(), scope);
    }

    scope->Set(stmt.name, value);
  }

  else if constexpr (std::is_same_v<T, ast::Assignment>) {
    auto value = VisitExpression(stmt.value, scope);

    if (const auto var_value = scope->Get(stmt.variable.name)) {
      scope->Set(stmt.variable.name, value);
    } else {
      throw MakeInterpreterError(
          std::format("Unknown variable: '{}'", stmt.variable.name), span);
    }
  }

  else if constexpr (std::is_same_v<T, ast::IfStatement>) {
    auto condition = VisitExpression(stmt.condition, scope);

    if (IsTruthy(condition)) {
      return VisitBlock(*stmt.then_branch, scope);
    }

    if (stmt.else_branch) {
      return VisitBlock(*stmt.else_branch, scope);
    }
  }

  else if constexpr (std::is_same_v<T, ast::ReturnStatement>) {
    if (stmt.expression.has_value()) {
      return VisitExpression(stmt.expression.value(), scope);
    }

    return std::nullopt;
  }

  else if constexpr (std::is_same_v<T, ast::ExpressionStatement>) {
    VisitExpression(stmt.expression, scope);
  }

  return std::nullopt;
}

}  // namespace lualike::interpreter

#endif  // LUALIKE_INTERPRETER_H_
