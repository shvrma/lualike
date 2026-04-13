#ifndef LUALIKE_PARSER_H_
#define LUALIKE_PARSER_H_

#include <algorithm>
#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "lualike/ast.h"
#include "lualike/lexer.h"
#include "lualike/value.h"

namespace lualike::parser {

struct ParserErr : std::runtime_error {
  explicit ParserErr(const std::string& msg) noexcept
      : std::runtime_error(msg) {}
};

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
class Parser {
  lexer::Lexer<InputT> lexer_;
  std::optional<token::Token> current_token_;

  bool IsEOF() const;
  const token::Token& Peek() const;
  token::Token Advance();
  token::Token Consume(token::TokenKind kind);
  bool Match(token::TokenKind kind);

  ast::Statement ParseStmt();
  ast::ReturnStatement ParseRetStmt();
  ast::VariableDeclaration ParseVarDecl(bool is_local);
  ast::IfStatement ParseIfStmt();
  ast::Block ParseBlock(std::initializer_list<token::TokenKind> end_tokens);
  ast::ExpressionStatement ParseExprStmt();
  ast::Expression ParseExpr(int min_precedence = 0);
  ast::Expression ParsePrimExpr();

 public:
  explicit Parser(InputT input)
      : lexer_(input), current_token_(lexer_.NextToken()) {}

  ast::Program Parse();
};

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
std::expected<ast::Program, ParserErr> Parse(InputT input) noexcept {
  try {
    Parser<InputT> parser(input);
    return parser.Parse();
  } catch (const ParserErr& err) {
    return std::unexpected(err);
  } catch (const std::exception& e) {
    return std::unexpected(ParserErr("Internal parser error"));
  } catch (...) {
    return std::unexpected(ParserErr("Unknown parser error"));
  }
}

inline const std::unordered_map<token::TokenKind, int> kBinOpsPrecedences = {
    {token::TokenKind::kKeywordOr, 1},
    {token::TokenKind::kKeywordAnd, 2},
    {token::TokenKind::kOtherLessThan, 3},
    {token::TokenKind::kOtherGreaterThan, 3},
    {token::TokenKind::kOtherLessThanEqual, 3},
    {token::TokenKind::kOtherGreaterThanEqual, 3},
    {token::TokenKind::kOtherTildeEqual, 3},
    {token::TokenKind::kOtherDoubleEqual, 3},
    {token::TokenKind::kOtherPlus, 9},
    {token::TokenKind::kOtherMinus, 9},
    {token::TokenKind::kOtherAsterisk, 10},
    {token::TokenKind::kOtherSlash, 10},
    {token::TokenKind::kOtherDoubleSlash, 10},
    {token::TokenKind::kOtherPercent, 10},
    {token::TokenKind::kOtherCaret, 11},
};

inline const std::unordered_map<token::TokenKind, ast::BinaryOperator>
    kTokenToBinOp = {
        {token::TokenKind::kKeywordOr, ast::BinaryOperator::kOr},
        {token::TokenKind::kKeywordAnd, ast::BinaryOperator::kAnd},
        {token::TokenKind::kOtherLessThan, ast::BinaryOperator::kLessThan},
        {token::TokenKind::kOtherGreaterThan,
         ast::BinaryOperator::kGreaterThan},
        {token::TokenKind::kOtherLessThanEqual,
         ast::BinaryOperator::kLessThanEqual},
        {token::TokenKind::kOtherGreaterThanEqual,
         ast::BinaryOperator::kGreaterThanEqual},
        {token::TokenKind::kOtherTildeEqual, ast::BinaryOperator::kNotEqual},
        {token::TokenKind::kOtherDoubleEqual, ast::BinaryOperator::kEqual},
        {token::TokenKind::kOtherPlus, ast::BinaryOperator::kAdd},
        {token::TokenKind::kOtherMinus, ast::BinaryOperator::kSubtract},
        {token::TokenKind::kOtherAsterisk, ast::BinaryOperator::kMultiply},
        {token::TokenKind::kOtherSlash, ast::BinaryOperator::kDivide},
        {token::TokenKind::kOtherDoubleSlash,
         ast::BinaryOperator::kFloorDivide},
        {token::TokenKind::kOtherPercent, ast::BinaryOperator::kModulo},
        {token::TokenKind::kOtherCaret, ast::BinaryOperator::kPower},
};

inline value::LualikeValue TokenToValue(const token::Token& token) {
  switch (token.token_kind) {
    case token::TokenKind::kStringLiteral: {
      const std::string_view data = token.source_span;
      return {std::string(data.substr(1, data.length() - 2))};
    }

    case token::TokenKind::kIntLiteral:
      return {std::stoi(std::string(token.source_span))};

    case token::TokenKind::kFloatLiteral:
      return {std::stod(std::string(token.source_span))};

    case token::TokenKind::kName:
      return {std::string(token.source_span)};

    case token::TokenKind::kKeywordTrue:
      return {true};

    case token::TokenKind::kKeywordFalse:
      return {false};

    case token::TokenKind::kKeywordNil:
      return {};

    default:
      throw ParserErr(std::format(
          "Unexpected token encountered during TokenToValue: kind {}",
          static_cast<int>(token.token_kind)));
  }
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::Program Parser<InputT>::Parse() {
  ast::Program program;
  auto& stmts = program.statements;

  while (!IsEOF()) {
    while (Match(token::TokenKind::kOtherSemicolon)) {
    }

    if (IsEOF()) {
      break;
    }

    stmts.push_back(ParseStmt());
  }

  return program;
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
bool Parser<InputT>::IsEOF() const {
  return !current_token_.has_value();
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
const token::Token& Parser<InputT>::Peek() const {
  if (IsEOF()) {
    throw ParserErr("Unexpected end of file encountered");
  }

  return *current_token_;
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
token::Token Parser<InputT>::Advance() {
  if (IsEOF()) {
    throw ParserErr("Unexpected end of file encountered while advancing");
  }

  auto token = *current_token_;
  current_token_ = lexer_.NextToken();
  return token;
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
token::Token Parser<InputT>::Consume(token::TokenKind kind) {
  const auto& token = Peek();
  if (token.token_kind != kind) {
    throw ParserErr(std::format(
        "Unexpected token expected kind {} but got {} with value {}",
        static_cast<int>(kind), static_cast<int>(token.token_kind),
        token.source_span));
  }

  return Advance();
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
bool Parser<InputT>::Match(token::TokenKind kind) {
  if (IsEOF() || Peek().token_kind != kind) {
    return false;
  }

  Advance();
  return true;
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::Statement Parser<InputT>::ParseStmt() {
  while (Match(token::TokenKind::kOtherSemicolon)) {
  }

  switch (Peek().token_kind) {
    case token::TokenKind::kName:
      return {ParseVarDecl(false)};
    case token::TokenKind::kKeywordReturn:
      return {ParseRetStmt()};
    case token::TokenKind::kKeywordLocal:
      return {ParseVarDecl(true)};
    case token::TokenKind::kKeywordIf:
      return {ParseIfStmt()};
    default:
      return {ParseExprStmt()};
  }
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::ReturnStatement Parser<InputT>::ParseRetStmt() {
  Advance();

  if (IsEOF() || Peek().token_kind == token::TokenKind::kOtherSemicolon) {
    return {std::nullopt};
  }

  return {ParseExpr()};
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::VariableDeclaration Parser<InputT>::ParseVarDecl(bool is_local) {
  if (is_local) {
    Consume(token::TokenKind::kKeywordLocal);
  }

  const auto name_token = Consume(token::TokenKind::kName);

  std::optional<ast::Expression> initializer;
  if (Match(token::TokenKind::kOtherEqual)) {
    initializer = ParseExpr();
  }

  return {std::string(name_token.source_span), std::move(initializer)};
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::IfStatement Parser<InputT>::ParseIfStmt() {
  Advance();

  auto condition = ParseExpr();

  Consume(token::TokenKind::kKeywordThen);
  auto then_branch = std::make_unique<ast::Block>(ParseBlock(
      {token::TokenKind::kKeywordElse, token::TokenKind::kKeywordEnd}));

  std::unique_ptr<ast::Block> else_branch;
  if (Match(token::TokenKind::kKeywordElse)) {
    else_branch = std::make_unique<ast::Block>(
        ParseBlock({token::TokenKind::kKeywordEnd}));
  }

  Consume(token::TokenKind::kKeywordEnd);

  return {std::move(condition), std::move(then_branch), std::move(else_branch)};
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::Block Parser<InputT>::ParseBlock(
    std::initializer_list<token::TokenKind> end_tokens) {
  ast::Block block;

  while (!IsEOF() &&
         std::ranges::find(end_tokens, Peek().token_kind) == end_tokens.end()) {
    while (Match(token::TokenKind::kOtherSemicolon)) {
    }

    if (IsEOF() ||
        std::ranges::find(end_tokens, Peek().token_kind) != end_tokens.end()) {
      break;
    }

    block.statements.push_back(ParseStmt());
  }

  return block;
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::ExpressionStatement Parser<InputT>::ParseExprStmt() {
  return {ParseExpr()};
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::Expression Parser<InputT>::ParseExpr(int min_precedence) {
  auto lhs = ParsePrimExpr();

  while (!IsEOF()) {
    const auto prec_it = kBinOpsPrecedences.find(Peek().token_kind);
    const int precedence =
        prec_it != kBinOpsPrecedences.end() ? prec_it->second : -1;
    if (precedence < min_precedence) {
      break;
    }

    auto op_token = Advance();
    const int next_precedence =
        op_token.token_kind == token::TokenKind::kOtherCaret ? precedence
                                                             : precedence + 1;
    auto rhs = ParseExpr(next_precedence);

    lhs.node = ast::BinaryExpression{
        kTokenToBinOp.at(op_token.token_kind),
        std::make_unique<ast::Expression>(std::move(lhs)),
        std::make_unique<ast::Expression>(std::move(rhs))};
  }

  return lhs;
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
ast::Expression Parser<InputT>::ParsePrimExpr() {
  const auto token = Advance();

  switch (token.token_kind) {
    case token::TokenKind::kStringLiteral:
    case token::TokenKind::kIntLiteral:
    case token::TokenKind::kFloatLiteral:
    case token::TokenKind::kKeywordTrue:
    case token::TokenKind::kKeywordFalse:
    case token::TokenKind::kKeywordNil:
      return {{ast::LiteralExpression{TokenToValue(token)}}};

    case token::TokenKind::kName:
      return {{ast::VariableExpression{std::string(token.source_span)}}};

    case token::TokenKind::kOtherMinus:
    case token::TokenKind::kKeywordNot: {
      constexpr int kUnaryPrecedence = 99;
      const auto oper = token.token_kind == token::TokenKind::kOtherMinus
                            ? ast::UnaryOperator::kNegate
                            : ast::UnaryOperator::kNot;
      return {{ast::UnaryExpression{oper, std::make_unique<ast::Expression>(
                                              ParseExpr(kUnaryPrecedence))}}};
    }

    case token::TokenKind::kOtherLeftParenthesis: {
      auto expr = ParseExpr();
      Consume(token::TokenKind::kOtherRightParenthesis);
      return expr;
    }

    default:
      throw ParserErr(
          std::format("Expected expression but found token {} with value {}",
                      static_cast<int>(token.token_kind), token.source_span));
  }
}

}  // namespace lualike::parser

#endif  // LUALIKE_PARSER_H_
