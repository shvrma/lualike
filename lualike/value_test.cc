#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ostream>

import lualike.value;

namespace lualike::value {

void PrintTo(const LualikeValue& value, std::ostream* output_stream) {
  *output_stream << value.ToString();
}

using LLV = LualikeValue;
using FloatT = LLV::FloatT;
using LLVOpErr = LualikeValueOpErr;
using testing::DoubleEq;
using testing::VariantWith;

TEST(OperatorsValidityTest, BinaryArithmeticOpsTest) {
  // Operations on ints.
  EXPECT_EQ(LLV{2} + LLV{3}, LLV{5});
  EXPECT_EQ(LLV{2} - LLV{3}, LLV{-1});
  EXPECT_EQ(LLV{2} * LLV{3}, LLV{6});
  EXPECT_FLOAT_EQ(std::get<LLV::FloatT>((LLV{2} / LLV{3}).inner_value),
                  2.0 / 3);
  EXPECT_EQ(LLV{5} % LLV{2}, LLV{1});
  EXPECT_THAT(LLV{2}.Exponentiate(LLV{3}).inner_value,
              VariantWith<FloatT>(DoubleEq(8.0)));

  // Operations on floats.
  EXPECT_EQ(LLV{2.0} + LLV{3.0}, LLV{5.0});
  EXPECT_EQ(LLV{2.0} - LLV{3.0}, LLV{-1.0});
  EXPECT_EQ(LLV{2.0} * LLV{3.0}, LLV{6.0});
  EXPECT_FLOAT_EQ(std::get<LLV::FloatT>((LLV{2.0} / LLV{3.0}).inner_value),
                  2.0 / 3);
  EXPECT_THROW({ LLV{2.0} % LLV{3.0}; }, LualikeValueOpErr);  // NOLINT
  EXPECT_THAT(LLV{2.0}.Exponentiate(LLV{3.0}).inner_value,
              VariantWith<FloatT>(DoubleEq(8.0)));

  // Operations on mixed numeric types.
  EXPECT_EQ(LLV{2} + LLV{3.0}, LLV{5.0});
  EXPECT_EQ(LLV{2.0} - LLV{3}, LLV{-1.0});
  EXPECT_EQ(LLV{2} * LLV{3.0}, LLV{6.0});
  EXPECT_FLOAT_EQ(std::get<LLV::FloatT>((LLV{2.0} / LLV{3}).inner_value),
                  2.0 / 3);
  EXPECT_THROW({ LLV{2} % LLV{3.0}; }, LualikeValueOpErr);  // NOLINT
  EXPECT_THAT(LLV{2.0}.Exponentiate(LLV{3}).inner_value,
              VariantWith<FloatT>(DoubleEq(8.0)));

  // Operations on mixed non-numeric types.
  EXPECT_THROW({ LLV{true} + LLV{1}; }, LLVOpErr);
  EXPECT_THROW(
      { LLV{true} - LLV{2.0}; },  // NOLINT
      LLVOpErr);
  EXPECT_THROW({ LLV{true} * LLV{3}; }, LLVOpErr);
  EXPECT_THROW(
      { LLV{true} / LLV{4.0}; },  // NOLINT
      LLVOpErr);
  EXPECT_THROW({ LLV{true} % LLV{5}; }, LLVOpErr);
  EXPECT_THROW({ LLV{true}.Exponentiate(LLV{1}); }, LLVOpErr);

  EXPECT_THROW({ LLV{-1} + LLV{"a"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{10} - LLV{"b"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{-1} * LLV{"c"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{10} / LLV{"d"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{-1} % LLV{"e"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{10} % LLV{"f"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{-1}.Exponentiate(LLV{"g"}); }, LLVOpErr);

  EXPECT_THROW({ LLV{true} + LLV{"m"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{false} - LLV{"e"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{true} * LLV{"s"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{false} / LLV{"s"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{true} % LLV{"a"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{false} % LLV{"g"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{true}.Exponentiate(LLV{"e"}); }, LLVOpErr);
}

TEST(OperatorsValidityTest, CumulativeBinaryArithmeticOpsTest) {
  LualikeValue lhs{5};  // NOLINT
  lhs += LualikeValue{1};
  ASSERT_EQ(lhs, LualikeValue{6});

  lhs -= LualikeValue{3};
  ASSERT_EQ(lhs, LualikeValue{3});

  lhs *= LualikeValue{4};
  ASSERT_EQ(lhs, LualikeValue{12});

  lhs /= LualikeValue{2};
  EXPECT_THAT(lhs.inner_value, VariantWith<FloatT>(DoubleEq(6.0)));

  lhs = {8};  // NOLINT. Should be int.
  lhs %= LualikeValue{3};
  ASSERT_EQ(lhs, LualikeValue{2});

  lhs.ExponentiateAndAssign(LualikeValue{3});
  EXPECT_THAT(lhs.inner_value, VariantWith<FloatT>(DoubleEq(8.0)));
}

TEST(OperatorsValidityTest, UnaryMinusTest) {
  ASSERT_EQ(-LualikeValue{-3}, LualikeValue{3});
  ASSERT_EQ(-LualikeValue{3.0}, LualikeValue{-3.0});

  EXPECT_THROW({ -LualikeValue{""}; }, LualikeValueOpErr);
}

TEST(OperatorsValidityTest, BinaryLogicalOpsTest) {
  const LualikeValue lhs{true};
  const LualikeValue rhs{false};

  ASSERT_EQ(lhs || rhs, LualikeValue{true});
  ASSERT_EQ(lhs && rhs, LualikeValue{false});
  ASSERT_EQ(!lhs, LualikeValue{false});

  EXPECT_THROW({ lhs || LualikeValue{1}; }, LualikeValueOpErr);
  EXPECT_THROW({ LualikeValue{2.0} || rhs; }, LualikeValueOpErr);  // NOLINT
  EXPECT_THROW({ LualikeValue{""} || LualikeValue{1}; }, LualikeValueOpErr);

  EXPECT_THROW({ lhs&& LualikeValue{1}; }, LualikeValueOpErr);
  EXPECT_THROW({ LualikeValue{2.0} && rhs; }, LualikeValueOpErr);  // NOLINT
  EXPECT_THROW({ LualikeValue{""} && LualikeValue{1}; }, LualikeValueOpErr);

  EXPECT_THROW({ !LualikeValue{""}; }, LualikeValueOpErr);
}

}  // namespace lualike::value
