#include "value.h"

#include <cmath>
#include <type_traits>

namespace value = lualike::value;

using value::LualikeValue;
using value::LualikeValueOpErr;
using value::LualikeValueOpErrKind;
using value::LualikeValueType;

// Helper functions.
static LualikeValue ToFloat(const LualikeValue &operand);
static LualikeValue PerformArithmeticOp(const LualikeValue &lhs,
                                        const LualikeValue &rhs,
                                        auto operation);

LualikeValueOpErr::LualikeValueOpErr(LualikeValueOpErrKind error_kind) noexcept
    : error_kind(error_kind) {}

inline LualikeValueType LualikeValue::GetValueType() const noexcept {
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

std::string LualikeValue::ToString() const noexcept {
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
      *this, rhs,
      []<typename OperandT>(const OperandT lhs, const OperandT rhs)
          -> OperandT { return lhs + rhs; });
}

LualikeValue LualikeValue::operator-(const LualikeValue &rhs) const {
  return PerformArithmeticOp(
      *this, rhs,
      []<typename OperandT>(const OperandT lhs, const OperandT rhs)
          -> OperandT { return lhs - rhs; });
}

LualikeValue LualikeValue::operator*(const LualikeValue &rhs) const {
  return PerformArithmeticOp(
      *this, rhs,
      []<typename OperandT>(const OperandT lhs, const OperandT rhs)
          -> OperandT { return lhs * rhs; });
}

LualikeValue LualikeValue::operator/(const LualikeValue &rhs) const {
  const auto lhs_as_float = ToFloat(*this);
  const auto rhs_as_float = ToFloat(rhs);

  return LualikeValue(LualikeValue::FloatT{
      std::get<LualikeValue::FloatT>(lhs_as_float.inner_value) /
      std::get<LualikeValue::FloatT>(rhs_as_float.inner_value)});
}

LualikeValue LualikeValue::FloorDivide(const LualikeValue &rhs) const {
  const auto division_rslt = *this / rhs;

  return LualikeValue(LualikeValue::FloatT{
      std::floor(std::get<LualikeValue::FloatT>(division_rslt.inner_value))});
}

LualikeValue LualikeValue::operator%(const LualikeValue &rhs) const {
  const auto division_result = *this % rhs;

  return LualikeValue(LualikeValue::FloatT{
      std::floor(std::get<LualikeValue::FloatT>(division_result.inner_value))});
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

  return LualikeValue(LualikeValue::FloatT{
      std::pow(std::get<LualikeValue::FloatT>(lhs_as_float.inner_value),
               std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))});
}

constexpr LualikeValue value::operator""_lua_int(unsigned long long int value) {
  return LualikeValue(static_cast<LualikeValue::IntT>(value));
}

constexpr LualikeValue value::operator""_lua_float(long double value) {
  return LualikeValue(static_cast<LualikeValue::FloatT>(value));
}

constexpr LualikeValue value::operator""_lua_str(const char *string,
                                                 std::size_t length) noexcept {
  return LualikeValue(LualikeValue::StringT(string, length));
}

LualikeValue ToFloat(const LualikeValue &operand) {
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

LualikeValue PerformArithmeticOp(const LualikeValue &lhs,
                                 const LualikeValue &rhs, auto operation) {
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
        return LualikeValue(LualikeValue::IntT{operation(
            lhs_value, std::get<LualikeValue::IntT>(rhs.inner_value))});
      }

      const LualikeValue lhs_as_float = to_float(lhs);
      const LualikeValue rhs_as_float = to_float(rhs);

      return LualikeValue(LualikeValue::FloatT{
          operation(std::get<LualikeValue::FloatT>(lhs_as_float.inner_value),
                    std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))});
    }

    if constexpr (std::is_same<T, LualikeValue::FloatT>()) {
      if (rhs.GetValueType() == LualikeValueType::kFloat) {
        return LualikeValue(LualikeValue::FloatT{operation(
            lhs_value, std::get<LualikeValue::FloatT>(rhs.inner_value))});
      }

      const LualikeValue rhs_as_float = to_float(rhs);
      return LualikeValue(LualikeValue::FloatT{
          operation(lhs_value,
                    std::get<LualikeValue::FloatT>(rhs_as_float.inner_value))});
    }

    else {
      throw LualikeValueOpErr(LualikeValueOpErrKind::kLhsOperandNotNumeric);
    }
  };

  return std::visit(visitor, lhs.inner_value);
}
