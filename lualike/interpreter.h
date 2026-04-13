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

class Interpreter {
  using NamesT = std::unordered_map<std::string, value::LualikeValue>;

  NamesT local_names_;
  std::shared_ptr<NamesT> global_names_;

  static bool IsTruthy(const value::LualikeValue& value);

  template <typename ExprT>
  value::LualikeValue ExprVisitor(const ExprT& expr);
  template <typename StmtT>
  std::optional<value::LualikeValue> StmtVisitor(const StmtT& stmt);

  value::LualikeValue VisitExpression(const ast::Expression& expression);
  std::optional<value::LualikeValue> VisitStatement(
      const ast::Statement& statement);
  std::optional<value::LualikeValue> VisitBlock(const ast::Block& block);

  explicit Interpreter(std::shared_ptr<NamesT> global_names)
      : global_names_(std::move(global_names)) {}

 public:
  template <std::ranges::view InputT>
    requires std::ranges::random_access_range<InputT>
  friend std::expected<std::optional<value::LualikeValue>, InterpreterErr>
  Interpret(InputT input) noexcept;
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

    auto interpreter = Interpreter(std::make_shared<Interpreter::NamesT>());
    return interpreter.VisitBlock(parse_result.value());
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

inline value::LualikeValue Interpreter::VisitExpression(
    const ast::Expression& expression) {
  return std::visit([this](const auto& expr) { return ExprVisitor(expr); },
                    expression.node);
}

inline std::optional<value::LualikeValue> Interpreter::VisitStatement(
    const ast::Statement& statement) {
  return std::visit([this](const auto& stmt) { return StmtVisitor(stmt); },
                    statement.node);
}

inline std::optional<value::LualikeValue> Interpreter::VisitBlock(
    const ast::Block& block) {
  NamesT original_locals = local_names_;
  for (const auto& statement : block.statements) {
    if (auto return_value = VisitStatement(statement)) {
      local_names_ = original_locals;
      return return_value;
    }
  }
  local_names_ = original_locals;

  return std::nullopt;
}

inline bool Interpreter::IsTruthy(const value::LualikeValue& value) {
  if (std::holds_alternative<value::LualikeValue::NilT>(value.inner_value)) {
    return false;
  }

  if (std::holds_alternative<value::LualikeValue::BoolT>(value.inner_value)) {
    return std::get<value::LualikeValue::BoolT>(value.inner_value);
  }

  return true;
}

template <typename ExprT>
value::LualikeValue Interpreter::ExprVisitor(const ExprT& expr) {
  using T = std::decay_t<decltype(expr)>;

  if constexpr (std::is_same_v<T, ast::LiteralExpression>) {
    return expr.value;
  } else if constexpr (std::is_same_v<T, ast::VariableExpression>) {
    if (const auto find_result = local_names_.find(expr.name);
        find_result != local_names_.end()) {
      return find_result->second;
    }

    if (const auto find_result = global_names_->find(expr.name);
        find_result != global_names_->end()) {
      return find_result->second;
    }

    throw InterpreterErr(std::format("Unknown variable: '{}'", expr.name));
  } else if constexpr (std::is_same_v<T, ast::UnaryExpression>) {
    auto rhs = VisitExpression(*expr.rhs);

    switch (expr.op) {
      case ast::UnaryOperator::kNegate:
        return -rhs;
      case ast::UnaryOperator::kNot:
        return !rhs;
    }

    throw InterpreterErr("Unimplemented unary operator");
  } else if constexpr (std::is_same_v<T, ast::BinaryExpression>) {
    auto lhs = VisitExpression(*expr.lhs);

    if (expr.op == ast::BinaryOperator::kAnd) {
      return IsTruthy(lhs) ? VisitExpression(*expr.rhs) : lhs;
    }
    if (expr.op == ast::BinaryOperator::kOr) {
      return IsTruthy(lhs) ? lhs : VisitExpression(*expr.rhs);
    }

    const auto rhs = VisitExpression(*expr.rhs);

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
  } else {
    throw InterpreterErr("Unimplemented expression type");
  }
}

template <typename StmtT>
std::optional<value::LualikeValue> Interpreter::StmtVisitor(const StmtT& stmt) {
  using T = std::decay_t<decltype(stmt)>;

  if constexpr (std::is_same_v<T, ast::VariableDeclaration>) {
    value::LualikeValue value;
    if (stmt.initializer) {
      value = VisitExpression(stmt.initializer.value());
    }
    const auto [_iter, was_successful] =
        local_names_.try_emplace(stmt.name, value);
    if (!was_successful) {
      throw InterpreterErr(
          std::format("Redeclaration of local variable: '{}'", stmt.name));
    }
  } else if constexpr (std::is_same_v<T, ast::Assignment>) {
    auto value = VisitExpression(stmt.value);

    if (local_names_.contains(stmt.variable.name)) {
      local_names_.at(stmt.variable.name) = value;
    } else {
      global_names_->insert_or_assign(stmt.variable.name, value);
    }
  } else if constexpr (std::is_same_v<T, ast::IfStatement>) {
    auto condition = VisitExpression(stmt.condition);

    if (IsTruthy(condition)) {
      return VisitBlock(*stmt.then_branch);
    }

    if (stmt.else_branch) {
      return VisitBlock(*stmt.else_branch);
    }
  } else if constexpr (std::is_same_v<T, ast::ReturnStatement>) {
    if (stmt.expression.has_value()) {
      return VisitExpression(stmt.expression.value());
    }

    return std::nullopt;
  } else if constexpr (std::is_same_v<T, ast::ExpressionStatement>) {
    VisitExpression(stmt.expression);
  }

  return std::nullopt;
}

}  // namespace lualike::interpreter

#endif  // LUALIKE_INTERPRETER_H_
