#include "lualike/ast.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace lualike::ast {

namespace {

void AppendIndent(std::string* out, int indent) {
  out->append(static_cast<size_t>(indent * 2), ' ');
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

void AppendToString(const Expression& expr, std::string* out, int indent);
void AppendToString(const Statement& stmt, std::string* out, int indent);
void AppendToString(const Block& block, std::string* out, int indent);
void AppendToString(const LiteralExpression& expr, std::string* out,
                    int indent);
void AppendToString(const VariableExpression& expr, std::string* out,
                    int indent);
void AppendToString(const UnaryExpression& expr, std::string* out, int indent);
void AppendToString(const BinaryExpression& expr, std::string* out, int indent);
void AppendToString(const FunctionCallExpression& expr, std::string* out,
                    int indent);
void AppendToString(const ExpressionStatement& stmt, std::string* out,
                    int indent);
void AppendToString(const ReturnStatement& stmt, std::string* out, int indent);
void AppendToString(const VariableDeclaration& stmt, std::string* out,
                    int indent);
void AppendToString(const Assignment& stmt, std::string* out, int indent);
void AppendToString(const IfStatement& stmt, std::string* out, int indent);
void AppendToString(const FunctionDeclaration& stmt, std::string* out,
                    int indent);

template <typename NodeT>
std::string ToIndentedString(const NodeT& node, int indent) {
  std::string out;
  AppendToString(node, &out, indent);
  return out;
}

template <typename NodeT>
void StreamNode(const NodeT& node, std::ostream* out, int indent) {
  *out << ToIndentedString(node, indent);
}

void AppendToString(const LiteralExpression& expr, std::string* out,
                    int indent) {
  AppendIndent(out, indent);
  out->append("LiteralExpression: ");
  out->append(expr.value.ToString());
  out->push_back('\n');
}

void AppendToString(const VariableExpression& expr, std::string* out,
                    int indent) {
  AppendIndent(out, indent);
  out->append("VariableExpression: ");
  out->append(expr.name);
  out->push_back('\n');
}

void AppendToString(const UnaryExpression& expr, std::string* out, int indent) {
  AppendIndent(out, indent);
  out->append("UnaryExpression: ");
  out->append(UnaryOperatorToString(expr.op));
  out->push_back('\n');
  AppendToString(*expr.rhs, out, indent + 1);
}

void AppendToString(const BinaryExpression& expr, std::string* out,
                    int indent) {
  AppendIndent(out, indent);
  out->append("BinaryExpression: ");
  out->append(BinaryOperatorToString(expr.op));
  out->push_back('\n');
  AppendToString(*expr.lhs, out, indent + 1);
  AppendToString(*expr.rhs, out, indent + 1);
}

void AppendToString(const FunctionCallExpression& expr, std::string* out,
                    int indent) {
  AppendIndent(out, indent);
  out->append("FunctionCallExpression\n");
  AppendIndent(out, indent + 1);
  out->append("Callee:\n");
  AppendToString(*expr.callee, out, indent + 2);
  AppendIndent(out, indent + 1);
  out->append("Arguments:\n");
  for (const auto& arg : expr.arguments) {
    AppendToString(arg, out, indent + 2);
  }
}

void AppendToString(const Expression& expr, std::string* out, int indent) {
  std::visit(
      [out, indent](const auto& node) { AppendToString(node, out, indent); },
      expr.node);
}

void AppendToString(const ExpressionStatement& stmt, std::string* out,
                    int indent) {
  AppendIndent(out, indent);
  out->append("ExpressionStatement\n");
  AppendToString(stmt.expression, out, indent + 1);
}

void AppendToString(const ReturnStatement& stmt, std::string* out, int indent) {
  AppendIndent(out, indent);
  out->append("ReturnStatement\n");
  if (stmt.expression) {
    AppendToString(*stmt.expression, out, indent + 1);
  }
}

void AppendToString(const VariableDeclaration& stmt, std::string* out,
                    int indent) {
  AppendIndent(out, indent);
  out->append("VariableDeclaration: ");
  out->append(stmt.name);
  out->push_back('\n');
  if (stmt.initializer) {
    AppendToString(*stmt.initializer, out, indent + 1);
  }
}

void AppendToString(const Assignment& stmt, std::string* out, int indent) {
  AppendIndent(out, indent);
  out->append("Assignment\n");
  AppendToString(stmt.variable, out, indent + 1);
  AppendToString(stmt.value, out, indent + 1);
}

void AppendToString(const IfStatement& stmt, std::string* out, int indent) {
  AppendIndent(out, indent);
  out->append("IfStatement\n");
  AppendIndent(out, indent + 1);
  out->append("Condition:\n");
  AppendToString(stmt.condition, out, indent + 2);
  AppendIndent(out, indent + 1);
  out->append("Then:\n");
  AppendToString(*stmt.then_branch, out, indent + 2);
  if (stmt.else_branch) {
    AppendIndent(out, indent + 1);
    out->append("Else:\n");
    AppendToString(*stmt.else_branch, out, indent + 2);
  }
}

void AppendToString(const FunctionDeclaration& stmt, std::string* out,
                    int indent) {
  AppendIndent(out, indent);
  out->append("FunctionDeclaration: ");
  out->append(stmt.name);
  out->push_back('(');
  for (size_t i = 0; i < stmt.params.size(); ++i) {
    out->append(stmt.params[i]);
    if (i < stmt.params.size() - 1) {
      out->append(", ");
    }
  }
  out->append(")\n");
  AppendToString(*stmt.body, out, indent + 1);
}

void AppendToString(const Statement& stmt, std::string* out, int indent) {
  std::visit(
      [out, indent](const auto& node) { AppendToString(node, out, indent); },
      stmt.node);
}

void AppendToString(const Block& block, std::string* out, int indent) {
  AppendIndent(out, indent);
  out->append("Block\n");
  for (const auto& stmt : block.statements) {
    AppendToString(stmt, out, indent + 1);
  }
}

}  // namespace

std::string ToString(const Expression& expr) {
  return ToIndentedString(expr, 0);
}

std::string ToString(const Statement& stmt) {
  return ToIndentedString(stmt, 0);
}

std::string ToString(const Block& block) { return ToIndentedString(block, 0); }

void PrintTo(const LiteralExpression& expr, std::ostream* out, int indent) {
  StreamNode(expr, out, indent);
}

void PrintTo(const VariableExpression& expr, std::ostream* out, int indent) {
  StreamNode(expr, out, indent);
}

void PrintTo(const UnaryExpression& expr, std::ostream* out, int indent) {
  StreamNode(expr, out, indent);
}

void PrintTo(const BinaryExpression& expr, std::ostream* out, int indent) {
  StreamNode(expr, out, indent);
}

void PrintTo(const FunctionCallExpression& expr, std::ostream* out,
             int indent) {
  StreamNode(expr, out, indent);
}

void PrintTo(const Expression& expr, std::ostream* out, int indent) {
  StreamNode(expr, out, indent);
}

void PrintTo(const ExpressionStatement& stmt, std::ostream* out, int indent) {
  StreamNode(stmt, out, indent);
}

void PrintTo(const ReturnStatement& stmt, std::ostream* out, int indent) {
  StreamNode(stmt, out, indent);
}

void PrintTo(const VariableDeclaration& stmt, std::ostream* out, int indent) {
  StreamNode(stmt, out, indent);
}

void PrintTo(const Assignment& stmt, std::ostream* out, int indent) {
  StreamNode(stmt, out, indent);
}

void PrintTo(const IfStatement& stmt, std::ostream* out, int indent) {
  StreamNode(stmt, out, indent);
}

void PrintTo(const FunctionDeclaration& stmt, std::ostream* out, int indent) {
  StreamNode(stmt, out, indent);
}

void PrintTo(const Statement& stmt, std::ostream* out, int indent) {
  StreamNode(stmt, out, indent);
}

void PrintTo(const Block& block, std::ostream* out, int indent) {
  StreamNode(block, out, indent);
}

void PrintTo(const Program& program, std::ostream* out) {
  *out << ToString(static_cast<const Block&>(program));
}

void PrintTo(const Statement& stmt, std::ostream* out) {
  *out << ToString(stmt);
}

void PrintTo(const Expression& expr, std::ostream* out) {
  *out << ToString(expr);
}

}  // namespace lualike::ast
