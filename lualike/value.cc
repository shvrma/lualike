#include "lualike/value.h"

#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace lualike::value {

namespace {

template <typename T>
std::optional<LualikeValue::FloatT> CastToFloat(const T& value) {
  using DecayedT = std::decay_t<T>;

  if constexpr (std::is_same_v<DecayedT, LualikeValue>) {
    return std::visit(
        [](const auto& inner_value) { return CastToFloat(inner_value); },
        value.inner_value);
  } else if constexpr (std::is_same_v<DecayedT, LualikeValue::IntT>) {
    return static_cast<LualikeValue::FloatT>(value);
  } else if constexpr (std::is_same_v<DecayedT, LualikeValue::FloatT>) {
    return value;
  } else {
    return std::nullopt;
  }
}

LualikeValue::BoolT TryAsBool(const LualikeValue& value,
                              LualikeValueOpErrKind error_kind) {
  try {
    return std::get<LualikeValue::BoolT>(value.inner_value);
  } catch (const std::bad_variant_access&) {
    throw LualikeValueOpErr(error_kind);
  }
}

template <typename OperatorT>
LualikeValue& PerformArithmeticBinOp(LualikeValue& lhs, const LualikeValue& rhs,
                                     const OperatorT& operator_lambda) {
  const auto visitor = [&operator_lambda,
                        &rhs](const auto& lhs_value) -> LualikeValue {
    using T = std::decay_t<decltype(lhs_value)>;

    if constexpr (std::is_same_v<T, LualikeValue::IntT>) {
      if (std::holds_alternative<LualikeValue::IntT>(rhs.inner_value)) {
        return {operator_lambda(lhs_value,
                                std::get<LualikeValue::IntT>(rhs.inner_value))};
      }

      const auto lhs_as_float = CastToFloat(lhs_value);
      const auto rhs_as_float = CastToFloat(rhs);
      if (!rhs_as_float) {
        throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotNumeric);
      }

      return {operator_lambda(*lhs_as_float, *rhs_as_float)};
    }

    if constexpr (std::is_same_v<T, LualikeValue::FloatT>) {
      const auto rhs_as_float = CastToFloat(rhs);
      if (!rhs_as_float) {
        throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotNumeric);
      }

      return {operator_lambda(lhs_value, *rhs_as_float)};
    }

    throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotNumeric);
  };

  lhs = std::visit(visitor, lhs.inner_value);
  return lhs;
}

}  // namespace

std::string LualikeValue::ToString() const {
  const auto visitor = [](const auto& value) -> std::string {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same_v<T, LualikeValue::NilT>) {
      return "Nil";
    } else if constexpr (std::is_same_v<T, LualikeValue::BoolT>) {
      return value ? "True" : "False";
    } else if constexpr (std::is_same_v<T, LualikeValue::IntT> ||
                         std::is_same_v<T, LualikeValue::FloatT>) {
      return "Number <" + std::to_string(value) + ">";
    } else if constexpr (std::is_same_v<T, LualikeValue::StringT>) {
      return "String <" + std::string{value} + ">";
    } else if constexpr (std::is_same_v<T, LualikeValue::FuncT>) {
      return "Function";
    } else {
      throw std::logic_error(
          "Unimplemented LualikeValue type in ToString visitor");
    }
  };

  return std::visit(visitor, inner_value);
}

LualikeValue& LualikeValue::operator+=(const LualikeValue& rhs) {
  return PerformArithmeticBinOp(
      *this, rhs,
      [](const auto lhs, const auto rhs_value) { return lhs + rhs_value; });
}

LualikeValue operator+(LualikeValue lhs, const LualikeValue& rhs) {
  lhs += rhs;
  return lhs;
}

LualikeValue& LualikeValue::operator-=(const LualikeValue& rhs) {
  return PerformArithmeticBinOp(
      *this, rhs,
      [](const auto lhs, const auto rhs_value) { return lhs - rhs_value; });
}

LualikeValue operator-(LualikeValue lhs, const LualikeValue& rhs) {
  lhs -= rhs;
  return lhs;
}

LualikeValue& LualikeValue::operator*=(const LualikeValue& rhs) {
  return PerformArithmeticBinOp(
      *this, rhs,
      [](const auto lhs, const auto rhs_value) { return lhs * rhs_value; });
}

LualikeValue operator*(LualikeValue lhs, const LualikeValue& rhs) {
  lhs *= rhs;
  return lhs;
}

LualikeValue& LualikeValue::operator/=(const LualikeValue& rhs) {
  const auto lhs_as_float = CastToFloat(*this);
  if (!lhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotNumeric);
  }

  const auto rhs_as_float = CastToFloat(rhs);
  if (!rhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotNumeric);
  }

  inner_value = *lhs_as_float / *rhs_as_float;
  return *this;
}

LualikeValue operator/(LualikeValue lhs, const LualikeValue& rhs) {
  lhs /= rhs;
  return lhs;
}

LualikeValue& LualikeValue::operator%=(const LualikeValue& rhs) {
  return PerformArithmeticBinOp(*this, rhs, [](auto lhs, const auto rhs_value) {
    return static_cast<LualikeValue::IntT>(
        std::floor(std::fmod(lhs, rhs_value)));
  });
}

LualikeValue operator%(LualikeValue lhs, const LualikeValue& rhs) {
  lhs %= rhs;
  return lhs;
}

LualikeValue& LualikeValue::ExponentiateAndAssign(const LualikeValue& rhs) {
  const auto lhs_as_float = CastToFloat(*this);
  if (!lhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotNumeric);
  }

  const auto rhs_as_float = CastToFloat(rhs);
  if (!rhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotNumeric);
  }

  inner_value = std::pow(*lhs_as_float, *rhs_as_float);
  return *this;
}

LualikeValue Exponentiate(LualikeValue lhs, const LualikeValue& rhs) {
  lhs.ExponentiateAndAssign(rhs);
  return lhs;
}

LualikeValue& LualikeValue::FloorDivideAndAssign(const LualikeValue& rhs) {
  const auto lhs_as_float = CastToFloat(*this);
  if (!lhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotNumeric);
  }

  const auto rhs_as_float = CastToFloat(rhs);
  if (!rhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotNumeric);
  }

  inner_value = std::floor(*lhs_as_float / *rhs_as_float);
  return *this;
}

LualikeValue FloorDivide(LualikeValue lhs, const LualikeValue& rhs) {
  lhs.FloorDivideAndAssign(rhs);
  return lhs;
}

LualikeValue operator-(const LualikeValue& operand) {
  const auto visitor = [](const auto& value) -> LualikeValue {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same_v<T, LualikeValue::IntT> ||
                  std::is_same_v<T, LualikeValue::FloatT>) {
      return {-value};
    }

    throw LualikeValueOpErr(LualikeValueOpErrKind::kNotNumericOperand);
  };

  return std::visit(visitor, operand.inner_value);
}

LualikeValue operator||(const LualikeValue& lhs, const LualikeValue& rhs) {
  const auto lhs_as_bool = TryAsBool(lhs, LualikeValueOpErrKind::kLhsNotBool);
  const auto rhs_as_bool = TryAsBool(rhs, LualikeValueOpErrKind::kRhsNotBool);

  return {lhs_as_bool || rhs_as_bool};
}

LualikeValue operator&&(const LualikeValue& lhs, const LualikeValue& rhs) {
  const auto lhs_as_bool = TryAsBool(lhs, LualikeValueOpErrKind::kLhsNotBool);
  const auto rhs_as_bool = TryAsBool(rhs, LualikeValueOpErrKind::kRhsNotBool);

  return {lhs_as_bool && rhs_as_bool};
}

LualikeValue operator!(const LualikeValue& operand) {
  const auto operand_as_bool =
      TryAsBool(operand, LualikeValueOpErrKind::kNotBoolOperand);

  return {!operand_as_bool};
}

void PrintTo(const LualikeValue& value, std::ostream* out) {
  *out << value.ToString();
}

}  // namespace lualike::value
