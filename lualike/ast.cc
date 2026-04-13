#include "lualike/ast.h"

#include <cstddef>
#include <ostream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>

namespace lualike::ast {

namespace {

void AppendIndent(std::ostream* out, int indent) {
  if (indent > 0) {
    std::print(*out, "{: >{}}", "", indent * 2);
  }
}

std::string_view UnaryOperatorToString(UnaryOperator oper) {
  switch (oper) {
    case UnaryOperator::kNegate:
      return "-";
    case UnaryOperator::kNot:
      return "not";

    default:
      throw std::runtime_error("Unknown UnaryOperator");
  }
}

std::string_view BinaryOperatorToString(BinaryOperator oper) {
  switch (oper) {
    case BinaryOperator::kOr:
      return "or";
    case BinaryOperator::kAnd:
      return "and";
    case BinaryOperator::kLessThan:
      return "<";
    case BinaryOperator::kGreaterThan:
      return ">";
    case BinaryOperator::kLessThanEqual:
      return "<=";
    case BinaryOperator::kGreaterThanEqual:
      return ">=";
    case BinaryOperator::kNotEqual:
      return "~=";
    case BinaryOperator::kEqual:
      return "==";
    case BinaryOperator::kAdd:
      return "+";
    case BinaryOperator::kSubtract:
      return "-";
    case BinaryOperator::kMultiply:
      return "*";
    case BinaryOperator::kDivide:
      return "/";
    case BinaryOperator::kFloorDivide:
      return "//";
    case BinaryOperator::kModulo:
      return "%";
    case BinaryOperator::kPower:
      return "^";

    default:
      throw std::runtime_error("Unknown BinaryOperator");
  }
}

}  // namespace

void PrintTo(const LiteralExpression& expr, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "LiteralExpression: {}", expr.value.ToString());
}

void PrintTo(const VariableExpression& expr, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "VariableExpression: {}", expr.name);
}

void PrintTo(const UnaryExpression& expr, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "UnaryExpression: {}", UnaryOperatorToString(expr.op));
  PrintTo(*expr.rhs, out, indent + 1);
}

void PrintTo(const BinaryExpression& expr, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "BinaryExpression: {}", BinaryOperatorToString(expr.op));
  PrintTo(*expr.lhs, out, indent + 1);
  PrintTo(*expr.rhs, out, indent + 1);
}

void PrintTo(const FunctionCallExpression& expr, std::ostream* out,
             int indent) {
  AppendIndent(out, indent);
  std::println(*out, "FunctionCallExpression");

  AppendIndent(out, indent + 1);
  std::println(*out, "Callee:");
  PrintTo(*expr.callee, out, indent + 2);

  AppendIndent(out, indent + 1);
  std::println(*out, "Arguments:");
  for (const auto& arg : expr.arguments) {
    PrintTo(arg, out, indent + 2);
  }
}

void PrintTo(const Expression& expr, std::ostream* out, int indent) {
  std::visit([out, indent](const auto& node) { PrintTo(node, out, indent); },
             expr.node);
}

void PrintTo(const ExpressionStatement& stmt, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "ExpressionStatement");
  PrintTo(stmt.expression, out, indent + 1);
}

void PrintTo(const ReturnStatement& stmt, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "ReturnStatement");
  if (stmt.expression) {
    PrintTo(*stmt.expression, out, indent + 1);
  }
}

void PrintTo(const VariableDeclaration& stmt, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "VariableDeclaration: {}", stmt.name);
  if (stmt.initializer) {
    PrintTo(*stmt.initializer, out, indent + 1);
  }
}

void PrintTo(const Assignment& stmt, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "Assignment");
  PrintTo(stmt.variable, out, indent + 1);
  PrintTo(stmt.value, out, indent + 1);
}

void PrintTo(const IfStatement& stmt, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "IfStatement");

  AppendIndent(out, indent + 1);
  std::println(*out, "Condition:");
  PrintTo(stmt.condition, out, indent + 2);

  AppendIndent(out, indent + 1);
  std::println(*out, "Then:");
  PrintTo(*stmt.then_branch, out, indent + 2);

  if (stmt.else_branch) {
    AppendIndent(out, indent + 1);
    std::println(*out, "Else:");
    PrintTo(*stmt.else_branch, out, indent + 2);
  }
}

void PrintTo(const FunctionDeclaration& stmt, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::println(*out, "FunctionDeclaration: {}({:n:s})", stmt.name, stmt.params);
  PrintTo(*stmt.body, out, indent + 1);
}

void PrintTo(const Statement& stmt, std::ostream* out, int indent) {
  std::visit([out, indent](const auto& node) { PrintTo(node, out, indent); },
             stmt.node);
}

void PrintTo(const Block& block, std::ostream* out, int indent) {
  AppendIndent(out, indent);
  std::print(*out, "Block\n");
  for (const auto& stmt : block.statements) {
    PrintTo(stmt, out, indent + 1);
  }
}

std::string ToString(const Expression& expr) {
  std::stringstream ss;
  PrintTo(expr, &ss, 0);
  return ss.str();
}

std::string ToString(const Statement& stmt) {
  std::stringstream ss;
  PrintTo(stmt, &ss, 0);
  return ss.str();
}

std::string ToString(const Block& block) {
  std::stringstream ss;
  PrintTo(block, &ss, 0);
  return ss.str();
}

void PrintTo(const Program& program, std::ostream* out) {
  PrintTo(program, out, 0);
}

void PrintTo(const Statement& stmt, std::ostream* out) {
  PrintTo(stmt, out, 0);
}

void PrintTo(const Expression& expr, std::ostream* out) {
  PrintTo(expr, out, 0);
}

}  // namespace lualike::ast
