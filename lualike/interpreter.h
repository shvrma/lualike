#ifndef LUALIKE_INTERPRETER_H_
#define LUALIKE_INTERPRETER_H_

#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

#include "lualike/ast.h"
#include "lualike/lexer.h"
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

template <typename ExprT>
value::LualikeValue ExprVisitor(const ExprT& expr,
                                std::shared_ptr<Scope> scope);
template <typename StmtT>
std::optional<value::LualikeValue> StmtVisitor(const StmtT& stmt,
                                               std::shared_ptr<Scope> scope);

static bool IsTruthy(const value::LualikeValue& value);

value::LualikeValue VisitExpression(const ast::Expression& expression,
                                    std::shared_ptr<Scope> scope);
std::optional<value::LualikeValue> VisitStatement(
    const ast::Statement& statement, std::shared_ptr<Scope> scope);
std::optional<value::LualikeValue> VisitBlock(const ast::Block& block,
                                              std::shared_ptr<Scope> scope);

struct InterpreterErr : std::runtime_error {
  std::exception_ptr internal_exception;

  explicit InterpreterErr(const std::string& msg) : std::runtime_error(msg) {}

  explicit InterpreterErr() noexcept
      : std::runtime_error("Internal interpreter error"),
        internal_exception(std::current_exception()) {}

  explicit InterpreterErr(const lexer::LexerErr& lexer_err) noexcept
      : std::runtime_error("Syntax error in input"),
        internal_exception(std::make_exception_ptr(lexer_err)) {}

  explicit InterpreterErr(const parser::ParserErr& parser_err) noexcept
      : std::runtime_error("Parser error in input"),
        internal_exception(std::make_exception_ptr(parser_err)) {}
};

template <std::ranges::view InputT>
  requires std::ranges::random_access_range<InputT>
std::expected<std::optional<value::LualikeValue>, InterpreterErr> Interpret(
    InputT input) noexcept {
  try {
    auto parse_result = parser::Parse(input);
    if (!parse_result) {
      return std::unexpected(InterpreterErr(parse_result.error()));
    }

    return VisitBlock(parse_result.value(), std::make_shared<Scope>());
  } catch (const InterpreterErr& err) {
    return std::unexpected(err);
  } catch (const lexer::LexerErr& err) {
    return std::unexpected(InterpreterErr(err));
  } catch (const std::exception& exception) {
    return std::unexpected(InterpreterErr());
  } catch (...) {
    return std::unexpected(InterpreterErr());
  }
}

inline value::LualikeValue VisitExpression(const ast::Expression& expression,
                                           std::shared_ptr<Scope> scope) {
  return std::visit(
      [scope](const auto& expr) { return ExprVisitor(expr, scope); },
      expression.node);
}

inline std::optional<value::LualikeValue> VisitStatement(
    const ast::Statement& statement, std::shared_ptr<Scope> scope) {
  return std::visit(
      [scope](const auto& stmt) { return StmtVisitor(stmt, scope); },
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
value::LualikeValue ExprVisitor(const ExprT& expr,
                                std::shared_ptr<Scope> scope) {
  using T = std::decay_t<decltype(expr)>;

  if constexpr (std::is_same_v<T, ast::LiteralExpression>) {
    return expr.value;
  }

  else if constexpr (std::is_same_v<T, ast::VariableExpression>) {
    if (const auto val = scope->Get(expr.name)) {
      return val.value();
    }

    throw InterpreterErr(std::format("Unknown variable: '{}'", expr.name));
  }

  else if constexpr (std::is_same_v<T, ast::UnaryExpression>) {
    auto rhs = VisitExpression(*expr.rhs, scope);

    switch (expr.op) {
      case ast::UnaryOperator::kNegate:
        return -rhs;
      case ast::UnaryOperator::kNot:
        return !rhs;
    }

    throw InterpreterErr("Unimplemented unary operator");
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
        return lhs + rhs;
      case ast::BinaryOperator::kSubtract:
        return lhs - rhs;
      case ast::BinaryOperator::kMultiply:
        return lhs * rhs;
      case ast::BinaryOperator::kDivide:
        return lhs / rhs;
      case ast::BinaryOperator::kFloorDivide:
        lhs.FloorDivideAndAssign(rhs);
        return lhs;
      case ast::BinaryOperator::kModulo:
        return lhs % rhs;
      case ast::BinaryOperator::kPower:
        lhs.ExponentiateAndAssign(rhs);
        return lhs;
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

    throw InterpreterErr("Unimplemented binary operator");
  }

  throw InterpreterErr("Unimplemented expression type");
}

template <typename StmtT>
std::optional<value::LualikeValue> StmtVisitor(const StmtT& stmt,
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
      throw InterpreterErr(
          std::format("Unknown variable: '{}'", stmt.variable.name));
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
