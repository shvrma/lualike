module;

#include <cmath>
#include <cstdint>
#include <string>
#include <variant>

export module lualike.value;

namespace lualike::value {

export enum class LualikeValueOpErrKind : uint8_t {
  kLhsOperandNotNumeric,
  kRhsOperandNotNumeric,
  kInvalidType,
};

// An exception type to be thrown in operations on LualikeValue. Clarification
// on what happened is given by _error_kind_ member, whose values are
// self-descriptive.
export struct LualikeValueOpErr : std::exception {
  LualikeValueOpErrKind error_kind;

  explicit LualikeValueOpErr(LualikeValueOpErrKind error_kind) noexcept;
};

export enum class LualikeValueType : uint8_t {
  kNil,
  kBool,
  kInt,
  kFloat,
  kString,
};

// Represents a single value within _lualike_. Note, though, that in _lualike_
// values have types, and each type has a corresponding C++ type associated with
// it. All the types are enumerated in the _LualikeValueType_ enumeration.
// Underlying C++ types are aliased as _*T_ within this struct (e.g.
// _Lualike::NilT_ is alias to lualike nil type).
export struct LualikeValue {
  using NilT = std::monostate;
  using BoolT = bool;
  using IntT = int64_t;
  using FloatT = double;
  using StringT = std::string;

  std::variant<NilT, BoolT, IntT, FloatT, StringT> inner_value = NilT{};

  bool operator==(const LualikeValue &) const = default;

  LualikeValueType GetValueType() const;
  std::string ToString() const;

  // Performs adding as this: if both operands are int, sums up integers;
  // otherwise if both are numeric interprets operands as floats and sums them
  // up.
  LualikeValue operator+(const LualikeValue &rhs) const;
  // Performs substraction similarly as summation operator do.
  LualikeValue operator-(const LualikeValue &rhs) const;
  // Performs multiplication similarly as summation operator do.
  LualikeValue operator*(const LualikeValue &rhs) const;
  // Performs division by first converting both operands to float.
  LualikeValue operator/(const LualikeValue &rhs) const;
  // Performs division by first converting both operands to float and then
  // rounding the result towards minus infinity.
  LualikeValue FloorDivide(const LualikeValue &rhs) const;
  // Performs modulo division similarly as summation operator do.
  LualikeValue operator%(const LualikeValue &rhs) const;
  // Exponantiates current value to RHS operator value by first conveerting them
  // both to float. Because of that exponents can be non-integer.
  LualikeValue Exponentiate(const LualikeValue &rhs) const;

  // Negates the number.
  LualikeValue operator-() const;

  // Performs logical AND on operands.
  LualikeValue operator&&(const LualikeValue &rhs) const;
  // Performs logical OR on operands.
  LualikeValue operator||(const LualikeValue &rhs) const;
  // Performs logical NOT on operands.
  LualikeValue operator!() const;
};

export LualikeValue operator""_lua_int(unsigned long long int value);
export LualikeValue operator""_lua_float(long double value);
export LualikeValue operator""_lua_str(const char *string,
                                       std::size_t length) noexcept;

static LualikeValue ToFloat(const LualikeValue &operand) {
  const auto visitor = [](auto &&value) -> LualikeValue {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LualikeValue::IntT>()) {
      return LualikeValue{static_cast<LualikeValue::FloatT>(value)};
    }

    else if constexpr (std::is_same<T, LualikeValue::FloatT>()) {
      return LualikeValue{value};

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
static LualikeValue PerformArithmeticOp(const LualikeValue &lhs,
                                        const LualikeValue &rhs,
                                        const OperatorLambdaT operator_lambda) {
  const auto to_float = [](const LualikeValue &value) {
    try {
      return ToFloat(value);
    } catch (LualikeValueOpErr &err) {
      throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsOperandNotNumeric);
    }
  };

  const auto visitor = [&](auto &&lhs_value) -> LualikeValue {
    using T = std::decay_t<decltype(lhs_value)>;

    if constexpr (std::is_same<T, LualikeValue::IntT>()) {
      if (rhs.GetValueType() == LualikeValueType::kInt) {
        return LualikeValue{LualikeValue::IntT{operator_lambda(
            lhs_value, std::get<LualikeValue::IntT>(rhs.inner_value))}};
      }

      const LualikeValue lhs_as_float = to_float(lhs);
      const LualikeValue rhs_as_float = to_float(rhs);

      return LualikeValue{LualikeValue::FloatT{operator_lambda(
          std::get<LualikeValue::FloatT>(lhs_as_float.inner_value),
          std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))}};
    }

    if constexpr (std::is_same<T, LualikeValue::FloatT>()) {
      if (rhs.GetValueType() == LualikeValueType::kFloat) {
        return LualikeValue{LualikeValue::FloatT{operator_lambda(
            lhs_value, std::get<LualikeValue::FloatT>(rhs.inner_value))}};
      }

      const LualikeValue rhs_as_float = to_float(rhs);
      return LualikeValue{LualikeValue::FloatT{operator_lambda(
          lhs_value,
          std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))}};
    }

    else {
      throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsOperandNotNumeric);
    }
  };

  return std::visit(visitor, lhs.inner_value);
}

LualikeValueOpErr::LualikeValueOpErr(LualikeValueOpErrKind error_kind) noexcept
    : error_kind(error_kind) {}

inline LualikeValueType LualikeValue::GetValueType() const {
  const auto visitor = [](auto &&value) -> LualikeValueType {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LualikeValue::NilT>()) {
      return LualikeValueType::kNil;
    }

    else if constexpr (std::is_same<T, LualikeValue::BoolT>()) {
      return LualikeValueType::kBool;
    }

    else if constexpr (std::is_same<T, LualikeValue::IntT>()) {
      return LualikeValueType::kInt;
    }

    else if constexpr (std::is_same<T, LualikeValue::FloatT>()) {
      return LualikeValueType::kFloat;
    }

    else if constexpr (std::is_same<T, LualikeValue::StringT>()) {
      return LualikeValueType::kString;
    }

    else {
      static_assert(false, "Not every type is covered.");
    }
  };

  return std::visit(visitor, LualikeValue::inner_value);
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

LualikeValue LualikeValue::operator+(const LualikeValue &rhs) const {
  return PerformArithmeticOp(
      *this, rhs, [](const auto lhs, const auto rhs) { return lhs + rhs; });
}

LualikeValue LualikeValue::operator-(const LualikeValue &rhs) const {
  return PerformArithmeticOp(
      *this, rhs, [](const auto lhs, const auto rhs) { return lhs - rhs; });
}

LualikeValue LualikeValue::operator*(const LualikeValue &rhs) const {
  return PerformArithmeticOp(
      *this, rhs, [](const auto lhs, const auto rhs) { return lhs * rhs; });
}

LualikeValue LualikeValue::operator/(const LualikeValue &rhs) const {
  const auto lhs_as_float = ToFloat(*this);
  const auto rhs_as_float = ToFloat(rhs);

  return LualikeValue{LualikeValue::FloatT{
      std::get<LualikeValue::FloatT>(lhs_as_float.inner_value) /
      std::get<LualikeValue::FloatT>(rhs_as_float.inner_value)}};
}

LualikeValue LualikeValue::FloorDivide(const LualikeValue &rhs) const {
  const auto division_rslt = *this / rhs;

  return LualikeValue{LualikeValue::FloatT{
      std::floor(std::get<LualikeValue::FloatT>(division_rslt.inner_value))}};
}

LualikeValue LualikeValue::operator%(const LualikeValue &rhs) const {
  const auto division_result = *this % rhs;

  return LualikeValue{LualikeValue::FloatT{
      std::floor(std::get<LualikeValue::FloatT>(division_result.inner_value))}};
}

LualikeValue LualikeValue::Exponentiate(const LualikeValue &rhs) const {
  const auto to_float = [](const LualikeValue &value) {
    try {
      return ToFloat(value);
    } catch (LualikeValueOpErr &err) {
      throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsOperandNotNumeric);
    }
  };

  const LualikeValue lhs_as_float = to_float(*this);
  const LualikeValue rhs_as_float = to_float(rhs);

  return LualikeValue{LualikeValue::FloatT{
      std::pow(std::get<LualikeValue::FloatT>(lhs_as_float.inner_value),
               std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))}};
}

}  // namespace lualike::value

namespace value = lualike::value;
using value::LualikeValue;

LualikeValue value::operator""_lua_int(unsigned long long value) {
  return LualikeValue{static_cast<LualikeValue::IntT>(value)};
}

LualikeValue value::operator""_lua_float(long double value) {
  return LualikeValue{static_cast<LualikeValue::FloatT>(value)};
}

LualikeValue value::operator""_lua_str(const char *string,
                                       std::size_t length) noexcept {
  return LualikeValue{LualikeValue::StringT(string, length)};
}
