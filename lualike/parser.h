#ifndef LUALIKE_PARSER_H_
#define LUALIKE_PARSER_H_

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <initializer_list>
#include <istream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "lualike/ast.h"
#include "lualike/error.h"
#include "lualike/lexer.h"
#include "lualike/value.h"

namespace lualike::parser {

inline error::Error MakeParserError(
    std::string message,
    std::optional<token::SourceSpan> context_span = std::nullopt) {
  return error::Error::Context(std::move(message), context_span);
}

namespace detail {

inline std::expected<std::string, error::Error> ReadStreamToString(
    std::istream& input) noexcept {
  try {
    std::string source(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) {
      return std::unexpected(
          error::Error::Message("Failed to read input stream"));
    }

    return source;
  } catch (const error::Error& err) {
    return std::unexpected(err);
  } catch (const std::exception&) {
    return std::unexpected(
        error::Error::FromCurrentException("Failed to read input stream"));
  } catch (...) {
    return std::unexpected(
        error::Error::Message("Failed to read input stream"));
  }
}

template <typename ResultT>
inline std::expected<ResultT, error::Error> AttachSourceText(
    std::expected<ResultT, error::Error> result, std::string source_text) {
  if (!result) {
    return std::unexpected(
        std::move(result).error().AttachSourceText(std::move(source_text)));
  }

  return result;
}

}  // namespace detail

class Parser {
  lexer::Lexer lexer_;
  std::optional<token::Token> current_token_;
  std::optional<token::Token> previous_token_;

  bool IsEOF() const;
  token::SourceSpan CurrentCursorSpan() const;
  token::SourceSpan SpanFrom(const token::Token& start_token) const;
  bool IsAtAnyOf(std::initializer_list<token::TokenKind> token_kinds) const;
  bool IsBlockTerminatedBy(const ast::Statement& statement) const;
  void ConsumeOptionalSemicolons();
  void EnsureBlockEndsAfterReturn(
      std::initializer_list<token::TokenKind> end_tokens = {});
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
  explicit Parser(std::string_view input)
      : lexer_(input), current_token_(lexer_.NextToken()) {}

  ast::Program Parse();
};

template <typename NodeT>
ast::Expression MakeExpression(NodeT node, token::SourceSpan span) {
  ast::Expression expression;
  expression.node = std::move(node);
  expression.span = span;
  return expression;
}

template <typename NodeT>
ast::Statement MakeStatement(NodeT node, token::SourceSpan span) {
  ast::Statement statement;
  statement.node = std::move(node);
  statement.span = span;
  return statement;
}

inline token::SourceSpan SpanFromStatements(
    const std::vector<ast::Statement>& statements,
    token::SourceSpan fallback = {}) {
  if (statements.empty()) {
    return fallback;
  }

  return token::MergeSourceSpans(statements.front().span,
                                 statements.back().span);
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
  try {
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
        throw MakeParserError(
            std::format(
                "Unexpected token encountered during TokenToValue: kind {}",
                static_cast<int>(token.token_kind)),
            token.span);
    }
  } catch (const error::Error&) {
    throw;
  } catch (...) {
    throw error::Error::FromCurrentException("Failed to parse literal value",
                                             token.span);
  }
}

namespace detail {

inline std::expected<ast::Program, error::Error> ParseSourceView(
    std::string_view input) noexcept {
  try {
    Parser parser(input);
    return parser.Parse();
  } catch (error::Error& err) {
    return std::unexpected(std::move(err));
  } catch (const std::exception&) {
    return std::unexpected(
        error::Error::FromCurrentException("Internal parser error"));
  } catch (...) {
    return std::unexpected(error::Error::Message("Unknown parser error"));
  }
}

}  // namespace detail

inline std::expected<ast::Program, error::Error> Parse(
    std::string_view input) noexcept {
  return detail::AttachSourceText(detail::ParseSourceView(input),
                                  std::string(input));
}

inline std::expected<ast::Program, error::Error> Parse(
    std::istream& input) noexcept {
  auto source_result = detail::ReadStreamToString(input);
  if (!source_result) {
    return std::unexpected(std::move(source_result).error());
  }

  std::string source = std::move(source_result).value();
  auto ast_result = detail::ParseSourceView(source);
  return detail::AttachSourceText(std::move(ast_result), std::move(source));
}

inline ast::Program Parser::Parse() {
  ast::Program program;
  auto& stmts = program.statements;

  while (!IsEOF()) {
    ConsumeOptionalSemicolons();

    if (IsEOF()) {
      break;
    }

    auto statement = ParseStmt();
    const bool terminates_program = IsBlockTerminatedBy(statement);
    stmts.push_back(std::move(statement));
    if (terminates_program) {
      EnsureBlockEndsAfterReturn();
      break;
    }
  }

  program.span = SpanFromStatements(program.statements, CurrentCursorSpan());
  return program;
}

inline bool Parser::IsEOF() const { return !current_token_.has_value(); }

inline token::SourceSpan Parser::CurrentCursorSpan() const {
  if (current_token_) {
    return current_token_->span;
  }
  if (previous_token_) {
    return {previous_token_->span.end, previous_token_->span.end};
  }

  return {};
}

inline token::SourceSpan Parser::SpanFrom(
    const token::Token& start_token) const {
  if (previous_token_) {
    return token::MergeSourceSpans(start_token.span, previous_token_->span);
  }

  return start_token.span;
}

inline bool Parser::IsAtAnyOf(
    std::initializer_list<token::TokenKind> token_kinds) const {
  return !IsEOF() &&
         std::ranges::find(token_kinds, Peek().token_kind) != token_kinds.end();
}

inline bool Parser::IsBlockTerminatedBy(const ast::Statement& statement) const {
  return std::holds_alternative<ast::ReturnStatement>(statement.node);
}

inline void Parser::ConsumeOptionalSemicolons() {
  while (Match(token::TokenKind::kOtherSemicolon)) {
  }
}

inline void Parser::EnsureBlockEndsAfterReturn(
    std::initializer_list<token::TokenKind> end_tokens) {
  ConsumeOptionalSemicolons();
  if (IsEOF() || IsAtAnyOf(end_tokens)) {
    return;
  }

  throw MakeParserError("Unexpected token after return statement", Peek().span);
}

inline const token::Token& Parser::Peek() const {
  if (IsEOF()) {
    throw MakeParserError("Unexpected end of file encountered",
                          CurrentCursorSpan());
  }

  return *current_token_;
}

inline token::Token Parser::Advance() {
  if (IsEOF()) {
    throw MakeParserError("Unexpected end of file encountered while advancing",
                          CurrentCursorSpan());
  }

  auto token = *current_token_;
  previous_token_ = token;
  current_token_ = lexer_.NextToken();
  return token;
}

inline token::Token Parser::Consume(token::TokenKind kind) {
  const auto& token = Peek();
  if (token.token_kind != kind) {
    throw MakeParserError(
        std::format(
            "Unexpected token expected kind {} but got {} with value {}",
            static_cast<int>(kind), static_cast<int>(token.token_kind),
            token.source_span),
        token.span);
  }

  return Advance();
}

inline bool Parser::Match(token::TokenKind kind) {
  if (IsEOF() || Peek().token_kind != kind) {
    return false;
  }

  Advance();
  return true;
}

inline ast::Statement Parser::ParseStmt() {
  ConsumeOptionalSemicolons();

  const auto start_token = Peek();
  switch (start_token.token_kind) {
    case token::TokenKind::kName:
      return MakeStatement(ParseVarDecl(false), SpanFrom(start_token));
    case token::TokenKind::kKeywordReturn:
      return MakeStatement(ParseRetStmt(), SpanFrom(start_token));
    case token::TokenKind::kKeywordLocal:
      return MakeStatement(ParseVarDecl(true), SpanFrom(start_token));
    case token::TokenKind::kKeywordIf:
      return MakeStatement(ParseIfStmt(), SpanFrom(start_token));
    default:
      return MakeStatement(ParseExprStmt(), SpanFrom(start_token));
  }
}

inline ast::ReturnStatement Parser::ParseRetStmt() {
  Advance();

  if (IsEOF() || Peek().token_kind == token::TokenKind::kOtherSemicolon) {
    return {std::nullopt};
  }

  return {ParseExpr()};
}

inline ast::VariableDeclaration Parser::ParseVarDecl(bool is_local) {
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

inline ast::IfStatement Parser::ParseIfStmt() {
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

inline ast::Block Parser::ParseBlock(
    std::initializer_list<token::TokenKind> end_tokens) {
  ast::Block block;

  while (!IsEOF() && !IsAtAnyOf(end_tokens)) {
    ConsumeOptionalSemicolons();

    if (IsEOF() || IsAtAnyOf(end_tokens)) {
      break;
    }

    auto statement = ParseStmt();
    const bool terminates_block = IsBlockTerminatedBy(statement);
    block.statements.push_back(std::move(statement));
    if (terminates_block) {
      EnsureBlockEndsAfterReturn(end_tokens);
      break;
    }
  }

  block.span = SpanFromStatements(block.statements, CurrentCursorSpan());
  return block;
}

inline ast::ExpressionStatement Parser::ParseExprStmt() {
  return {ParseExpr()};
}

inline ast::Expression Parser::ParseExpr(int min_precedence) {
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
    const auto expression_span = token::MergeSourceSpans(lhs.span, rhs.span);

    lhs = MakeExpression(
        ast::BinaryExpression{
            kTokenToBinOp.at(op_token.token_kind),
            std::make_unique<ast::Expression>(std::move(lhs)),
            std::make_unique<ast::Expression>(std::move(rhs))},
        expression_span);
  }

  return lhs;
}

inline ast::Expression Parser::ParsePrimExpr() {
  const auto token = Advance();

  switch (token.token_kind) {
    case token::TokenKind::kStringLiteral:
    case token::TokenKind::kIntLiteral:
    case token::TokenKind::kFloatLiteral:
    case token::TokenKind::kKeywordTrue:
    case token::TokenKind::kKeywordFalse:
    case token::TokenKind::kKeywordNil:
      return MakeExpression(ast::LiteralExpression{TokenToValue(token)},
                            token.span);

    case token::TokenKind::kName:
      return MakeExpression(
          ast::VariableExpression{std::string(token.source_span)}, token.span);

    case token::TokenKind::kOtherMinus:
    case token::TokenKind::kKeywordNot: {
      constexpr int kUnaryPrecedence = 99;
      const auto oper = token.token_kind == token::TokenKind::kOtherMinus
                            ? ast::UnaryOperator::kNegate
                            : ast::UnaryOperator::kNot;
      auto rhs = ParseExpr(kUnaryPrecedence);
      const auto expression_span =
          token::MergeSourceSpans(token.span, rhs.span);
      return MakeExpression(
          ast::UnaryExpression{
              oper, std::make_unique<ast::Expression>(std::move(rhs))},
          expression_span);
    }

    case token::TokenKind::kOtherLeftParenthesis: {
      auto expr = ParseExpr();
      const auto right_paren =
          Consume(token::TokenKind::kOtherRightParenthesis);
      expr.span = token::MergeSourceSpans(token.span, right_paren.span);
      return expr;
    }

    default:
      throw MakeParserError(
          std::format("Expected expression but found token {} with value {}",
                      static_cast<int>(token.token_kind), token.source_span),
          token.span);
  }
}

}  // namespace lualike::parser

#endif  // LUALIKE_PARSER_H_
