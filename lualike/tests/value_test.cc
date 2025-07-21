#include <gmock/gmock.h>
#include <gtest/gtest.h>

import lualike.value;

namespace lualike::value {

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
  EXPECT_EQ(LLV{2} / LLV{3}, LLV{2.0 / 3});
  EXPECT_EQ(LLV{5} % LLV{2}, LLV{1});
  EXPECT_EQ(Exponentiate(LLV{2}, LLV{3}), LLV{8.0});

  // Operations on floats.
  EXPECT_EQ(LLV{2.0} + LLV{3.0}, LLV{5.0});
  EXPECT_EQ(LLV{2.0} - LLV{3.0}, LLV{-1.0});
  EXPECT_EQ(LLV{2.0} * LLV{3.0}, LLV{6.0});
  EXPECT_EQ(LLV{2.0} / LLV{3.0}, LLV{2.0 / 3});
  EXPECT_EQ(LLV{2.0} % LLV{3.0}, LLV{2});
  EXPECT_THAT(Exponentiate(LLV{2.0}, LLV{3.0}), LLV{8.0});

  // Operations on mixed numeric types.
  EXPECT_EQ(LLV{2} + LLV{3.0}, LLV{5.0});
  EXPECT_EQ(LLV{2.0} - LLV{3}, LLV{-1.0});
  EXPECT_EQ(LLV{2} * LLV{3.0}, LLV{6.0});
  EXPECT_EQ(LLV{2.0} / LLV{3}, LLV{2.0 / 3});
  EXPECT_EQ(LLV{2} % LLV{3.0}, LLV{2});
  EXPECT_EQ(Exponentiate(LLV{2.0}, LLV{3}), LLV{8.0});

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
  EXPECT_THROW({ Exponentiate(LLV{true}, LLV{1}); }, LLVOpErr);

  EXPECT_THROW({ LLV{-1} + LLV{"a"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{10} - LLV{"b"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{-1} * LLV{"c"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{10} / LLV{"d"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{-1} % LLV{"e"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{10} % LLV{"f"}; }, LLVOpErr);
  EXPECT_THROW({ Exponentiate(LLV{-1}, LLV{"g"}); }, LLVOpErr);

  EXPECT_THROW({ LLV{true} + LLV{"m"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{false} - LLV{"e"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{true} * LLV{"s"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{false} / LLV{"s"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{true} % LLV{"a"}; }, LLVOpErr);
  EXPECT_THROW({ LLV{false} % LLV{"g"}; }, LLVOpErr);
  EXPECT_THROW({ Exponentiate(LLV{true}, LLV{"e"}); }, LLVOpErr);
}

TEST(OperatorsValidityTest, CumulativeBinaryArithmeticOpsTest) {
  LLV lhs{5};
  lhs += LLV{1};
  ASSERT_EQ(lhs, LLV{6});

  lhs -= LLV{3};
  ASSERT_EQ(lhs, LLV{3});

  lhs *= LLV{4};
  ASSERT_EQ(lhs, LLV{12});

  lhs /= LLV{2};
  EXPECT_THAT(lhs.inner_value, VariantWith<FloatT>(DoubleEq(6.0)));

  lhs = {8};
  lhs %= LLV{3};
  ASSERT_EQ(lhs, LLV{2});

  lhs.ExponentiateAndAssign(LLV{3});
  EXPECT_THAT(lhs.inner_value, VariantWith<FloatT>(DoubleEq(8.0)));
}

TEST(OperatorsValidityTest, UnaryMinusTest) {
  ASSERT_EQ(-LLV{-3}, LLV{3});
  ASSERT_EQ(-LLV{3.0}, LLV{-3.0});

  EXPECT_THROW({ -LLV{""}; }, LualikeValueOpErr);
}

TEST(OperatorsValidityTest, BinaryLogicalOpsTest) {
  const LLV lhs{true};
  const LLV rhs{false};

  ASSERT_EQ(lhs || rhs, LLV{true});
  ASSERT_EQ(lhs && rhs, LLV{false});
  ASSERT_EQ(!lhs, LLV{false});

  EXPECT_THROW({ lhs || LLV{1}; }, LualikeValueOpErr);
  EXPECT_THROW({ LLV{2.0} || rhs; }, LualikeValueOpErr);
  EXPECT_THROW({ LLV{""} || LLV{1}; }, LualikeValueOpErr);

  EXPECT_THROW({ lhs&& LLV{1}; }, LualikeValueOpErr);
  EXPECT_THROW({ LLV{2.0} && rhs; }, LualikeValueOpErr);
  EXPECT_THROW({ LLV{""} && LLV{1}; }, LualikeValueOpErr);

  EXPECT_THROW({ !LLV{""}; }, LualikeValueOpErr);
}

}  // namespace lualike::value
