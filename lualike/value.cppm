module;

#include <cmath>
#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

export module lualike.value;

export namespace lualike::value {

enum class LualikeValueOpErrKind : uint8_t {
  kInvalidType,
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
struct LualikeValue {
  using NilT = std::monostate;
  using BoolT = bool;
  using IntT = int64_t;
  using FloatT = double;
  using StringT = std::string;

  std::variant<NilT, BoolT, IntT, FloatT, StringT> inner_value{NilT{}};

  bool operator==(const LualikeValue &rhs) const = default;
  auto operator<=>(const LualikeValue &rhs) const = default;

  std::string ToString() const;

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
  // Performs division by first converting both operands to float and then
  // rounding the result towards minus infinity.
  friend LualikeValue FloorDivide(const LualikeValue &lhs,
                                  const LualikeValue &rhs);
  // Performs modulo division similarly as summation operator do.
  friend LualikeValue operator%(const LualikeValue &lhs,
                                const LualikeValue &rhs);
  // Exponantiates current value to RHS operator value by first conveerting them
  // both to float. Because of that exponents can be non-integer.
  friend LualikeValue Exponentiate(const LualikeValue &lhs,
                                   const LualikeValue &rhs);

  // Negates the number.
  // LualikeValue operator-() const;

  // Performs logical AND on operands.
  // LualikeValue operator&&(const LualikeValue &rhs) const;
  // Performs logical OR on operands.
  // LualikeValue operator||(const LualikeValue &rhs) const;
  // Performs logical NOT on operands.
  // LualikeValue operator!() const;
};

static_assert(std::is_aggregate<LualikeValue>());

LualikeValue operator""_lua_int(unsigned long long value);
LualikeValue operator""_lua_float(long double value);
LualikeValue operator""_lua_str(const char *string,
                                std::size_t length) noexcept;

}  // namespace lualike::value

namespace lualike::value {

LualikeValue ToFloat(const LualikeValue &operand) {
  const auto visitor = [](auto &&value) -> LualikeValue {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LualikeValue::IntT>()) {
      return {static_cast<LualikeValue::FloatT>(value)};
    }

    else if constexpr (std::is_same<T, LualikeValue::FloatT>()) {
      return {value};
    }

    else if constexpr (std::is_same<T, LualikeValue::NilT>() ||
                       std::is_same<T, LualikeValue::BoolT>() ||
                       std::is_same<T, LualikeValue::StringT>()) {
      throw LualikeValueOpErr(LualikeValueOpErrKind::kInvalidType);
    }

    else {
      static_assert(false, "Not every type is covered.");
    }
  };

  return std::visit(visitor, operand.inner_value);
}

template <typename OperatorLambdaT>
LualikeValue PerformArithmeticOp(const LualikeValue &lhs,
                                 const LualikeValue &rhs,
                                 const OperatorLambdaT operator_lambda) {
  const auto visitor = [&](auto &&lhs_value) -> LualikeValue {
    using T = std::decay_t<decltype(lhs_value)>;

    if constexpr (std::is_same<T, LualikeValue::IntT>()) {
      if (std::holds_alternative<LualikeValue::IntT>(rhs.inner_value)) {
        return {operator_lambda(lhs_value,
                                std::get<LualikeValue::IntT>(rhs.inner_value))};
      }

      const LualikeValue lhs_as_float = ToFloat(lhs);
      const LualikeValue rhs_as_float = ToFloat(rhs);

      return LualikeValue{LualikeValue::FloatT{operator_lambda(
          std::get<LualikeValue::FloatT>(lhs_as_float.inner_value),
          std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))}};
    }

    if constexpr (std::is_same<T, LualikeValue::FloatT>()) {
      const LualikeValue rhs_as_float = ToFloat(rhs);

      return {operator_lambda(
          lhs_value, std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))};
    }

    else {
      throw LualikeValueOpErr(LualikeValueOpErrKind::kInvalidType);
    }
  };

  return std::visit(visitor, lhs.inner_value);
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

LualikeValue operator+(const LualikeValue &lhs, const LualikeValue &rhs) {
  return PerformArithmeticOp(
      lhs, rhs, [](const auto lhs, const auto rhs) { return lhs + rhs; });
}

LualikeValue operator-(const LualikeValue &lhs, const LualikeValue &rhs) {
  return PerformArithmeticOp(
      lhs, rhs, [](const auto lhs, const auto rhs) { return lhs - rhs; });
}

LualikeValue operator*(const LualikeValue &lhs, const LualikeValue &rhs) {
  return PerformArithmeticOp(
      lhs, rhs, [](const auto lhs, const auto rhs) { return lhs * rhs; });
}

LualikeValue operator/(const LualikeValue &lhs, const LualikeValue &rhs) {
  const LualikeValue lhs_as_float = ToFloat(lhs);
  const LualikeValue rhs_as_float = ToFloat(rhs);

  return {LualikeValue::FloatT{
      std::get<LualikeValue::FloatT>(lhs_as_float.inner_value) /
      std::get<LualikeValue::FloatT>(rhs_as_float.inner_value)}};
}

LualikeValue FloorDivide(const LualikeValue &lhs, const LualikeValue &rhs) {
  const auto division_result = lhs / rhs;

  return {static_cast<LualikeValue::IntT>(
      std::floor(std::get<LualikeValue::FloatT>(division_result.inner_value)))};
}

LualikeValue operator%(const LualikeValue &lhs, const LualikeValue &rhs) {
  try {
    return {static_cast<LualikeValue::FloatT>(
        std::get<LualikeValue::IntT>(lhs.inner_value) %
        std::get<LualikeValue::IntT>(rhs.inner_value))};
  } catch (std::bad_variant_access &) {
    throw LualikeValueOpErr(LualikeValueOpErrKind::kInvalidType);
  }
}

LualikeValue Exponentiate(const LualikeValue &lhs, const LualikeValue &rhs) {
  const LualikeValue lhs_as_float = ToFloat(lhs);
  const LualikeValue rhs_as_float = ToFloat(rhs);

  return {LualikeValue::FloatT{
      std::pow(std::get<LualikeValue::FloatT>(lhs_as_float.inner_value),
               std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))}};
}

LualikeValue operator""_lua_int(unsigned long long value) {
  return {static_cast<LualikeValue::IntT>(value)};
}

LualikeValue operator""_lua_float(long double value) {
  return {static_cast<LualikeValue::FloatT>(value)};
}

LualikeValue operator""_lua_str(const char *string,
                                std::size_t length) noexcept {
  return {LualikeValue::StringT(string, length)};
}

}  // namespace lualike::value
