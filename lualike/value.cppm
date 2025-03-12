module;

#include <cmath>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

export module lualike.value;

export namespace lualike::value {

enum class LualikeValueOpErrKind : uint8_t {
  kInvalidOperandType,
  kNormallyImpossibleErr,
  kLhsNotNumeric,
  kLhsNotBool,
  kRhsNotNumeric,
  kRhsNotBool,
};

// An exception type to be thrown in operations on LualikeValue. Clarification
// on what happened is given by _error_kind_ member, whose values are
// self-descriptive.
struct LualikeValueOpErr : std::exception {
  LualikeValueOpErrKind error_kind;

  explicit LualikeValueOpErr(LualikeValueOpErrKind error_kind) noexcept
      : error_kind(error_kind) {}
};

// Represents a single value within _lualike_. Note, though, that in _lualike_
// values have types, and each type has a corresponding C++ type associated with
// it. All the types are enumerated in the _LualikeValueType_ enumeration.
// Underlying C++ types are aliased as _*T_ within this struct (e.g.
// _Lualike::NilT_ is alias to lualike nil type).
class LualikeValue {
  LualikeValue PerformArithmeticBinOp(this LualikeValue &lhs,
                                      const LualikeValue &rhs,
                                      auto operator_lambda);

 public:
  using NilT = std::monostate;
  using BoolT = bool;
  using IntT = int64_t;
  using FloatT = double;
  using StringT = std::string;

  std::variant<NilT, BoolT, IntT, FloatT, StringT> inner_value{NilT{}};

  friend bool operator==(const LualikeValue &, const LualikeValue &) = default;
  friend auto operator<=>(const LualikeValue &, const LualikeValue &) = default;

  std::string ToString() const;

  LualikeValue operator+=(this LualikeValue &lhs, const LualikeValue &rhs);
  LualikeValue operator-=(this LualikeValue &lhs, const LualikeValue &rhs);
  LualikeValue operator*=(this LualikeValue &lhs, const LualikeValue &rhs);
  LualikeValue operator/=(this LualikeValue &lhs, const LualikeValue &rhs);
  LualikeValue operator%=(this LualikeValue &lhs, const LualikeValue &rhs);
  LualikeValue ExponentiateAndAssign(this LualikeValue &lhs,
                                     const LualikeValue &rhs);

  // Performs adding as this: if both operands are int, sums up integers;
  // otherwise if both are numeric interprets operands as floats and sums them
  // up.
  friend LualikeValue operator+(const LualikeValue &lhs,
                                const LualikeValue &rhs);
  // Performs substraction similarly as summation operator do.
  friend LualikeValue operator-(const LualikeValue &lhs,
                                const LualikeValue &rhs);
  // Performs multiplication similarly as summation operator do.
  friend LualikeValue operator*(const LualikeValue &lhs,
                                const LualikeValue &rhs);
  // Performs division by first converting both operands to float.
  friend LualikeValue operator/(const LualikeValue &lhs,
                                const LualikeValue &rhs);
  // Performs modulo division similarly as summation operator do.
  friend LualikeValue operator%(const LualikeValue &lhs,
                                const LualikeValue &rhs);
  // Exponantiates current value to RHS operator value by first conveerting them
  // both to float. Because of that exponents can be non-integer.
  LualikeValue Exponentiate(this const LualikeValue &lhs,
                            const LualikeValue &rhs);

  // Negates the number.
  LualikeValue operator-(this const LualikeValue &operand);

  // Performs logical OR on operands.
  friend LualikeValue operator||(const LualikeValue &lhs,
                                 const LualikeValue &rhs);
  // Performs logical AND on operands.
  friend LualikeValue operator&&(const LualikeValue &lhs,
                                 const LualikeValue &rhs);
  // Performs logical NOT on operands.
  LualikeValue operator!(this const LualikeValue &operand);
};

}  // namespace lualike::value

namespace lualike::value {

std::optional<LualikeValue::FloatT> CastToFloat(const auto &value) {
  using T = std::decay_t<decltype(value)>;

  if constexpr (std::is_same<T, LualikeValue>()) {
    return std::visit([](const auto &value) { return CastToFloat(value); },
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

std::optional<LualikeValue::BoolT> CastToBool(const LualikeValue &value) {
  return std::visit(
      [](const auto &value) -> std::optional<LualikeValue::BoolT> {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same<T, LualikeValue::BoolT>()) {
          return value;
        }

        else {
          return std::nullopt;
        }
      },
      value.inner_value);
}

inline LualikeValue LualikeValue::PerformArithmeticBinOp(
    this LualikeValue &lhs, const LualikeValue &rhs,
    const auto operator_lambda) {
  const auto visitor = [operator_lambda,
                        &rhs](const auto &lhs_value) -> LualikeValue {
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
      return std::to_string(value);
    }

    else if constexpr (std::is_same<T, LualikeValue::StringT>()) {
      return std::string{value};
    }

    else {
      static_assert(false, "Not every type is covered.");
    }
  };

  return std::visit(visitor, LualikeValue::inner_value);
}

LualikeValue LualikeValue::operator+=(this LualikeValue &lhs,
                                      const LualikeValue &rhs) {
  return lhs.PerformArithmeticBinOp(
      rhs, [](auto &lhs, const auto rhs) { return lhs + rhs; });
}

LualikeValue operator+(const LualikeValue &lhs, const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result += rhs;
  return result;
}

LualikeValue LualikeValue::operator-=(this LualikeValue &lhs,
                                      const LualikeValue &rhs) {
  return lhs.PerformArithmeticBinOp(
      rhs, [](auto &lhs, const auto rhs) { return lhs - rhs; });
}

LualikeValue operator-(const LualikeValue &lhs, const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result -= rhs;
  return result;
}

LualikeValue LualikeValue::operator*=(this LualikeValue &lhs,
                                      const LualikeValue &rhs) {
  return lhs.PerformArithmeticBinOp(
      rhs, [](auto &lhs, const auto rhs) { return lhs * rhs; });
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

LualikeValue LualikeValue::Exponentiate(this const LualikeValue &lhs,
                                        const LualikeValue &rhs) {
  LualikeValue result = lhs;
  result.ExponentiateAndAssign(rhs);
  return result;
}

LualikeValue LualikeValue ::operator-(this const LualikeValue &operand) {
  return std::visit(
      [](const auto &operand_value) -> LualikeValue {
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
  const auto lhs_as_bool = CastToBool(lhs);
  if (!lhs_as_bool) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotBool);
  }

  const auto rhs_as_bool = CastToBool(rhs);
  if (!rhs_as_bool) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotBool);
  }

  return {*lhs_as_bool || *rhs_as_bool};
}

LualikeValue operator&&(const LualikeValue &lhs, const LualikeValue &rhs) {
  const auto lhs_as_bool = CastToBool(lhs);
  if (!lhs_as_bool) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsNotBool);
  }

  const auto rhs_as_bool = CastToBool(rhs);
  if (!rhs_as_bool) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotBool);
  }

  return {*lhs_as_bool && *rhs_as_bool};
}

LualikeValue LualikeValue::operator!(this const LualikeValue &operand) {
  const auto operand_as_bool = CastToBool(operand);
  if (!operand_as_bool) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kRhsNotBool);
  }

  return {!*operand_as_bool};
}

}  // namespace lualike::value
