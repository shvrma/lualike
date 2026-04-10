#ifndef LUALIKE_AST_H_
#define LUALIKE_AST_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include "lualike/value.h"

namespace lualike::ast {

struct Expression;
struct Statement;
struct Block;

enum class UnaryOperator : uint8_t { kNegate, kNot };
enum class BinaryOperator : uint8_t {
  kOr,
  kAnd,
  kLessThan,
  kGreaterThan,
  kLessThanEqual,
  kGreaterThanEqual,
  kNotEqual,
  kEqual,
  kAdd,
  kSubtract,
  kMultiply,
  kDivide,
  kFloorDivide,
  kModulo,
  kPower,
};

struct LiteralExpression {
  value::LualikeValue value;
};

struct VariableExpression {
  std::string name;
};

struct UnaryExpression {
  UnaryOperator op;
  std::unique_ptr<Expression> rhs;
};

struct BinaryExpression {
  BinaryOperator op;
  std::unique_ptr<Expression> lhs;
  std::unique_ptr<Expression> rhs;
};

struct FunctionCallExpression {
  std::unique_ptr<Expression> callee;
  std::vector<Expression> arguments;
};

struct Expression {
  std::variant<LiteralExpression, VariableExpression, UnaryExpression,
               BinaryExpression, FunctionCallExpression>
      node;
};

struct ExpressionStatement {
  Expression expression;
};

struct ReturnStatement {
  std::optional<Expression> expression;
};

struct VariableDeclaration {
  std::string name;
  std::optional<Expression> initializer;
};

struct Assignment {
  VariableExpression variable;
  Expression value;
};

struct IfStatement {
  Expression condition;
  std::unique_ptr<Block> then_branch;
  std::unique_ptr<Block> else_branch;
};

struct FunctionDeclaration {
  std::string name;
  std::vector<std::string> params;
  std::unique_ptr<Block> body;
};

struct Statement {
  std::variant<ExpressionStatement, ReturnStatement, VariableDeclaration,
               Assignment, IfStatement, FunctionDeclaration>
      node;
};

struct Block {
  std::vector<Statement> statements;
};

using Program = Block;

std::string ToString(const Expression& expr);
std::string ToString(const Statement& stmt);
std::string ToString(const Block& block);

void PrintTo(const Expression& expr, std::ostream* out, int indent);
void PrintTo(const Statement& stmt, std::ostream* out, int indent);
void PrintTo(const Block& block, std::ostream* out, int indent);
void PrintTo(const LiteralExpression& expr, std::ostream* out, int indent);
void PrintTo(const VariableExpression& expr, std::ostream* out, int indent);
void PrintTo(const UnaryExpression& expr, std::ostream* out, int indent);
void PrintTo(const BinaryExpression& expr, std::ostream* out, int indent);
void PrintTo(const FunctionCallExpression& expr, std::ostream* out, int indent);
void PrintTo(const ExpressionStatement& stmt, std::ostream* out, int indent);
void PrintTo(const ReturnStatement& stmt, std::ostream* out, int indent);
void PrintTo(const VariableDeclaration& stmt, std::ostream* out, int indent);
void PrintTo(const Assignment& stmt, std::ostream* out, int indent);
void PrintTo(const IfStatement& stmt, std::ostream* out, int indent);
void PrintTo(const FunctionDeclaration& stmt, std::ostream* out, int indent);
void PrintTo(const Program& program, std::ostream* out);
void PrintTo(const Statement& stmt, std::ostream* out);
void PrintTo(const Expression& expr, std::ostream* out);

}  // namespace lualike::ast

#endif  // LUALIKE_AST_H_
