#include <gtest/gtest.h>

#include <generator>
#include <string_view>
#include <utility>

import lualike.interpreter;

namespace lualike::value {

void PrintTo(const LualikeValue& value, std::ostream* output_stream) {
  *output_stream << value.ToString();
}

}  // namespace lualike::value

namespace lualike::interpreter {

using lualike::lexer::Lexer;

namespace value = lualike::value;
using value::LualikeValue;
using value::operator""_lua_int;
using value::operator""_lua_float;
using value::operator""_lua_str;

using InterpreterTestParamT = std::pair<std::string_view, LualikeValue>;

class InterpreterTest : public testing::TestWithParam<InterpreterTestParamT> {};

TEST_P(InterpreterTest, EvaluateAndCompareWithValid) {
  const auto& [input, expected_result] = InterpreterTest::GetParam();

  Lexer<std::string_view> lexer(input);
  auto tokens_r = lexer.ReadTokens();
  auto interpreter = Interpreter(tokens_r);

  const auto actual_result = interpreter.EvaluateExpression();

  ASSERT_EQ(actual_result, expected_result)
      << "Actual result: " << actual_result.ToString();
}

const InterpreterTestParamT kSingleDigit = {"1", 1_lua_int};

const InterpreterTestParamT kMultipleDigits = {"11111", 11111_lua_int};

const InterpreterTestParamT kFloat = {"3.14", 3.14_lua_float};

const InterpreterTestParamT kSimpleExpression = {"1 + 2 + 3", 6_lua_int};

const InterpreterTestParamT kHarderExpression = {"2 + 2 * 2", 6_lua_int};

INSTANTIATE_TEST_SUITE_P(TestValidInputs, InterpreterTest,
                         testing::Values(kSingleDigit, kMultipleDigits,
                                         kSimpleExpression, kHarderExpression));

}  // namespace lualike::interpreter
