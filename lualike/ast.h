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

  bool operator==(const LiteralExpression&) const = default;
};

struct VariableExpression {
  std::string name;

  bool operator==(const VariableExpression&) const = default;
};

struct UnaryExpression {
  UnaryOperator op;
  std::shared_ptr<Expression> rhs;

  bool operator==(const UnaryExpression& other) const;
};

struct BinaryExpression {
  BinaryOperator op;
  std::shared_ptr<Expression> lhs;
  std::shared_ptr<Expression> rhs;

  bool operator==(const BinaryExpression& other) const;
};

struct FunctionCallExpression {
  std::shared_ptr<Expression> callee;
  std::vector<Expression> arguments;

  bool operator==(const FunctionCallExpression& other) const;
};

struct Expression {
  std::variant<LiteralExpression, VariableExpression, UnaryExpression,
               BinaryExpression, FunctionCallExpression>
      node;

  bool operator==(const Expression&) const = default;
};

struct ExpressionStatement {
  Expression expression;

  bool operator==(const ExpressionStatement&) const = default;
};

struct ReturnStatement {
  std::optional<Expression> expression;

  bool operator==(const ReturnStatement&) const = default;
};

struct VariableDeclaration {
  std::string name;
  std::optional<Expression> initializer;

  bool operator==(const VariableDeclaration&) const = default;
};

struct Assignment {
  VariableExpression variable;
  Expression value;

  bool operator==(const Assignment&) const = default;
};

struct IfStatement {
  Expression condition;
  std::shared_ptr<Block> then_branch;
  std::shared_ptr<Block> else_branch;

  bool operator==(const IfStatement& other) const;
};

struct FunctionDeclaration {
  std::string name;
  std::vector<std::string> params;
  std::shared_ptr<Block> body;

  bool operator==(const FunctionDeclaration& other) const;
};

struct Statement {
  std::variant<ExpressionStatement, ReturnStatement, VariableDeclaration,
               Assignment, IfStatement, FunctionDeclaration>
      node;

  bool operator==(const Statement&) const = default;
};

struct Block {
  std::vector<Statement> statements;

  bool operator==(const Block&) const = default;
};

using Program = Block;

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
