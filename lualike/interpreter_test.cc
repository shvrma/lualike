#include "interpreter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "value.h"

namespace lualike::interpreter {

using value::LuaValue;

using value::operator""_lua_int;
using value::operator""_lua_float;
using value::operator""_lua_str;

using InterpreterTestParamT = std::pair<std::string, EvaluateResult>;

class InterpreterTest : public testing::TestWithParam<InterpreterTestParamT> {};

TEST_P(InterpreterTest, ReadAndCompareWithGiven) {
  const auto &[input, expected_evaluation_result] = InterpreterTest::GetParam();

  std::istringstream prog{input};
  auto interpreter = Interpreter(prog);

  const auto actual_evaluation_result = interpreter.Evaluate();

  ASSERT_EQ(actual_evaluation_result, expected_evaluation_result);
}

INSTANTIATE_TEST_SUITE_P(
    TestValidInputs, InterpreterTest,
    testing::ValuesIn(std::to_array<InterpreterTestParamT>({
        {"1", 1_lua_int},
        {"1 + 2", 3_lua_int},
        {"(1 + 2) + 3", 6_lua_int},
        {"2 * 2", 4_lua_int},
        {"(2 * 2)", 4_lua_int},
        {"(2 * 2) + 2", 6_lua_int},
        {"2 * 2 + 2", 6_lua_int},
        {"6 / 2", 3_lua_int},
    })));

// INSTANTIATE_TEST_SUITE_P(
//     TestInvalidInputs, InterpreterTest,
//     testing::ValuesIn(std::to_array<InterpreterTestParamT>({
//         {"+", InterpreterErr::kInvalidToken},
//         // {"1 +", InterpreterErr::kInvalidToken},
//     })));

}  // namespace lualike::interpreter
