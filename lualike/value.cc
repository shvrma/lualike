module;

#include <cmath>
#include <expected>
#include <optional>
#include <print>
#include <string>
#include <type_traits>
#include <variant>

module lualike.value;

namespace lualike::value {

std::optional<LualikeValue::FloatT> CastToFloat(const auto &value) {
  using T = std::decay_t<decltype(value)>;

  if constexpr (std::is_same<T, LualikeValue>()) {
    return std::visit([](auto &&value) { return CastToFloat(value); },
                      value.inner_value);
  }

  else if constexpr (std::is_same<T, LualikeValue::IntT>()) {
    return static_cast<LualikeValue::FloatT>(value);
  }

  else if constexpr (std::is_same<T, LualikeValue::FloatT>()) {
    return value;
  }

  else {
    // TODO(shvrma): string conv.
    return std::nullopt;
  }
}

LualikeValue::BoolT TryAsBool(const LualikeValue &value,
                              const LualikeValueOpErrKind &err_kind) {
  try {
    return std::get<LualikeValue::BoolT>(value.inner_value);
  } catch (std::bad_variant_access &err) {
    throw LualikeValueOpErr(err_kind);
  }
};

inline LualikeValue PerformArithmeticBinOp(LualikeValue &lhs,
                                           const LualikeValue &rhs,
                                           const auto &operator_lambda) {
  const auto visitor = [operator_lambda,
                        &rhs](auto &&lhs_value) -> LualikeValue {
    using T = std::decay_t<decltype(lhs_value)>;

    if constexpr (std::is_same<T, LualikeValue::IntT>()) {
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

    if constexpr (std::is_same<T, LualikeValue::FloatT>()) {
      const auto rhs_as_float = CastToFloat(rhs);
      if (!rhs_as_float) {
        throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotNumeric);
      }

      return {operator_lambda(lhs_value, *rhs_as_float)};
    }

    else {
      throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotNumeric);
    }
  };

  return lhs = std::visit(visitor, lhs.inner_value);
}

std::string LualikeValue::ToString() const {
  const auto visitor = [](auto &&value) -> std::string {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LualikeValue::NilT>()) {
      return "nil";
    }

    else if constexpr (std::is_same<T, LualikeValue::BoolT>()) {
      if (value) {
        return "true";
      }

      return "false";
    }

    else if constexpr (std::is_same<T, LualikeValue::IntT>() ||
                       std::is_same<T, LualikeValue::FloatT>()) {
      return "Number <" + std::to_string(value) + ">";
    }

    else if constexpr (std::is_same<T, LualikeValue::StringT>()) {
      return "String <" + std::string{value} + ">";
    }

    else if constexpr (std::is_same<T, LualikeValue::FuncT>()) {
      return "function obj";
    }

    else {
      static_assert(false, "Not every type is covered.");
    }
  };

  return std::visit(visitor, LualikeValue::inner_value);
}

LualikeValue LualikeValue::operator+=(this LualikeValue &lhs,
                                      const LualikeValue &rhs) {
  return PerformArithmeticBinOp(
      lhs, rhs, [](auto &lhs, const auto rhs) { return lhs + rhs; });
}

LualikeValue operator+(const LualikeValue &lhs, const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result += rhs;
  return result;
}

LualikeValue LualikeValue::operator-=(this LualikeValue &lhs,
                                      const LualikeValue &rhs) {
  return PerformArithmeticBinOp(
      lhs, rhs, [](auto &lhs, const auto rhs) { return lhs - rhs; });
}

LualikeValue operator-(const LualikeValue &lhs, const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result -= rhs;
  return result;
}

LualikeValue LualikeValue::operator*=(this LualikeValue &lhs,
                                      const LualikeValue &rhs) {
  return PerformArithmeticBinOp(
      lhs, rhs, [](auto &lhs, const auto rhs) { return lhs * rhs; });
}

LualikeValue operator*(const LualikeValue &lhs, const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result *= rhs;
  return result;
}

LualikeValue LualikeValue::operator/=(this LualikeValue &lhs,
                                      const LualikeValue &rhs) {
  const auto lhs_as_float = CastToFloat(lhs);
  if (!lhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotNumeric);
  }

  const auto rhs_as_float = CastToFloat(rhs);
  if (!rhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotNumeric);
  }

  return lhs = {*lhs_as_float / *rhs_as_float};
}

LualikeValue operator/(const LualikeValue &lhs, const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result /= rhs;
  return result;
}

LualikeValue LualikeValue::operator%=(this LualikeValue &lhs,
                                      const LualikeValue &rhs) {
  try {
    lhs.inner_value =
        LualikeValue::IntT{std::get<LualikeValue::IntT>(lhs.inner_value) %
                           std::get<LualikeValue::IntT>(rhs.inner_value)};
  } catch (std::bad_variant_access &) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kInvalidOperandType);
  }

  return lhs;
}

LualikeValue operator%(const LualikeValue &lhs, const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result %= rhs;
  return result;
}

LualikeValue LualikeValue::ExponentiateAndAssign(this LualikeValue &lhs,
                                                 const LualikeValue &rhs) {
  const auto lhs_as_float = CastToFloat(lhs);
  if (!lhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotNumeric);
  }

  const auto rhs_as_float = CastToFloat(rhs);
  if (!rhs_as_float) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotNumeric);
  }

  lhs.inner_value =
      LualikeValue::FloatT{std::pow(*lhs_as_float, *rhs_as_float)};
  return lhs;
}

LualikeValue Exponentiate(const LualikeValue &lhs, const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result.ExponentiateAndAssign(rhs);
  return result;
}

LualikeValue operator-(const LualikeValue &operand) {
  return std::visit(
      [](auto &&operand_value) -> LualikeValue {
        using T = std::decay_t<decltype(operand_value)>;

        if constexpr (std::is_same<T, LualikeValue::IntT>() ||
                      std::is_same<T, LualikeValue::FloatT>()) {
          return {-operand_value};
        }

        else {
          throw LualikeValueOpErr(LualikeValueOpErrKind::kInvalidOperandType);
        }
      },
      operand.inner_value);
}

LualikeValue operator||(const LualikeValue &lhs, const LualikeValue &rhs) {
  const auto lhs_as_bool = TryAsBool(lhs, LualikeValueOpErrKind::kLhsNotBool);
  const auto rhs_as_bool = TryAsBool(rhs, LualikeValueOpErrKind::kLhsNotBool);

  return {lhs_as_bool || rhs_as_bool};
}

LualikeValue operator&&(const LualikeValue &lhs, const LualikeValue &rhs) {
  const auto lhs_as_bool = TryAsBool(lhs, LualikeValueOpErrKind::kLhsNotBool);
  const auto rhs_as_bool = TryAsBool(rhs, LualikeValueOpErrKind::kLhsNotBool);

  return {lhs_as_bool && rhs_as_bool};
}

LualikeValue operator!(const LualikeValue &operand) {
  const auto operand_as_bool =
      TryAsBool(operand, LualikeValueOpErrKind::kLhsNotBool);

  return {!operand_as_bool};
}

}  // namespace lualike::value
