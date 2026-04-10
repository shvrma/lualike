#include "lualike/ast.h"

#include <cstddef>
#include <map>
#include <string>
#include <string_view>

namespace {

template <typename T>
bool SharedPtrEqual(const std::shared_ptr<T>& lhs,
                    const std::shared_ptr<T>& rhs) {
  if (!lhs || !rhs) {
    return lhs == rhs;
  }

  return *lhs == *rhs;
}

}  // namespace

namespace lualike::ast {

bool UnaryExpression::operator==(const UnaryExpression& other) const {
  return op == other.op && SharedPtrEqual(rhs, other.rhs);
}

bool BinaryExpression::operator==(const BinaryExpression& other) const {
  return op == other.op && SharedPtrEqual(lhs, other.lhs) &&
         SharedPtrEqual(rhs, other.rhs);
}

bool FunctionCallExpression::operator==(
    const FunctionCallExpression& other) const {
  return SharedPtrEqual(callee, other.callee) && arguments == other.arguments;
}

bool IfStatement::operator==(const IfStatement& other) const {
  return condition == other.condition &&
         SharedPtrEqual(then_branch, other.then_branch) &&
         SharedPtrEqual(else_branch, other.else_branch);
}

bool FunctionDeclaration::operator==(const FunctionDeclaration& other) const {
  return name == other.name && params == other.params &&
         SharedPtrEqual(body, other.body);
}

namespace {

void PrintIndent(std::ostream* out, int indent) {
  *out << std::string(static_cast<size_t>(indent * 2), ' ');
}

std::ostream& operator<<(std::ostream& out, UnaryOperator oper) {
  static const std::map<UnaryOperator, std::string_view> kUnaryOperatorMap = {
      {UnaryOperator::kNegate, "-"},
      {UnaryOperator::kNot, "not"},
  };

  if (const auto iter = kUnaryOperatorMap.find(oper);
      iter != kUnaryOperatorMap.end()) {
    return out << iter->second;
  }

  return out << "<unknown_unary_op>";
}

std::ostream& operator<<(std::ostream& out, BinaryOperator oper) {
  static const std::map<BinaryOperator, std::string_view> kBinaryOperatorMap = {
      {BinaryOperator::kOr, "or"},
      {BinaryOperator::kAnd, "and"},
      {BinaryOperator::kLessThan, "<"},
      {BinaryOperator::kGreaterThan, ">"},
      {BinaryOperator::kLessThanEqual, "<="},
      {BinaryOperator::kGreaterThanEqual, ">="},
      {BinaryOperator::kNotEqual, "~="},
      {BinaryOperator::kEqual, "=="},
      {BinaryOperator::kAdd, "+"},
      {BinaryOperator::kSubtract, "-"},
      {BinaryOperator::kMultiply, "*"},
      {BinaryOperator::kDivide, "/"},
      {BinaryOperator::kFloorDivide, "//"},
      {BinaryOperator::kModulo, "%"},
      {BinaryOperator::kPower, "^"},
  };

  if (const auto iter = kBinaryOperatorMap.find(oper);
      iter != kBinaryOperatorMap.end()) {
    return out << iter->second;
  }

  return out << "<unknown_binary_op>";
}

}  // namespace

void PrintTo(const LiteralExpression& expr, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "LiteralExpression: " << expr.value.ToString() << "\n";
}

void PrintTo(const VariableExpression& expr, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "VariableExpression: " << expr.name << "\n";
}

void PrintTo(const UnaryExpression& expr, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "UnaryExpression: " << expr.op << "\n";
  PrintTo(*expr.rhs, out, indent + 1);
}

void PrintTo(const BinaryExpression& expr, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "BinaryExpression: " << expr.op << "\n";
  PrintTo(*expr.lhs, out, indent + 1);
  PrintTo(*expr.rhs, out, indent + 1);
}

void PrintTo(const FunctionCallExpression& expr, std::ostream* out,
             int indent) {
  PrintIndent(out, indent);
  *out << "FunctionCallExpression\n";
  PrintIndent(out, indent + 1);
  *out << "Callee:\n";
  PrintTo(*expr.callee, out, indent + 2);
  PrintIndent(out, indent + 1);
  *out << "Arguments:\n";
  for (const auto& arg : expr.arguments) {
    PrintTo(arg, out, indent + 2);
  }
}

void PrintTo(const Expression& expr, std::ostream* out, int indent) {
  std::visit([out, indent](const auto& node) { PrintTo(node, out, indent); },
             expr.node);
}

void PrintTo(const ExpressionStatement& stmt, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "ExpressionStatement\n";
  PrintTo(stmt.expression, out, indent + 1);
}

void PrintTo(const ReturnStatement& stmt, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "ReturnStatement\n";
  if (stmt.expression) {
    PrintTo(*stmt.expression, out, indent + 1);
  }
}

void PrintTo(const VariableDeclaration& stmt, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "VariableDeclaration: " << stmt.name << "\n";
  if (stmt.initializer) {
    PrintTo(*stmt.initializer, out, indent + 1);
  }
}

void PrintTo(const Assignment& stmt, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "Assignment\n";
  PrintTo(stmt.variable, out, indent + 1);
  PrintTo(stmt.value, out, indent + 1);
}

void PrintTo(const IfStatement& stmt, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "IfStatement\n";
  PrintIndent(out, indent + 1);
  *out << "Condition:\n";
  PrintTo(stmt.condition, out, indent + 2);
  PrintIndent(out, indent + 1);
  *out << "Then:\n";
  PrintTo(*stmt.then_branch, out, indent + 2);
  if (stmt.else_branch) {
    PrintIndent(out, indent + 1);
    *out << "Else:\n";
    PrintTo(*stmt.else_branch, out, indent + 2);
  }
}

void PrintTo(const FunctionDeclaration& stmt, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "FunctionDeclaration: " << stmt.name << "(";
  for (size_t i = 0; i < stmt.params.size(); ++i) {
    *out << stmt.params[i];
    if (i < stmt.params.size() - 1) {
      *out << ", ";
    }
  }
  *out << ")\n";
  PrintTo(*stmt.body, out, indent + 1);
}

void PrintTo(const Statement& stmt, std::ostream* out, int indent) {
  std::visit([out, indent](const auto& node) { PrintTo(node, out, indent); },
             stmt.node);
}

void PrintTo(const Block& block, std::ostream* out, int indent) {
  PrintIndent(out, indent);
  *out << "Block\n";
  for (const auto& stmt : block.statements) {
    PrintTo(stmt, out, indent + 1);
  }
}

void PrintTo(const Program& program, std::ostream* out) {
  *out << "\n";
  PrintTo(static_cast<const Block&>(program), out, 0);
}

void PrintTo(const Statement& stmt, std::ostream* out) {
  PrintTo(stmt, out, 0);
}

void PrintTo(const Expression& expr, std::ostream* out) {
  PrintTo(expr, out, 0);
}

}  // namespace lualike::ast
