#include "value.h"

#include <charconv>
#include <cmath>

namespace value = lualike::value;

using value::ArithmeticOpErr;
using value::LuaValue;
using value::LuaValueKind;
using value::TryMakeErr;

constexpr LuaValue::LuaValue(decltype(inner_value_) &&value_) noexcept
    : inner_value_(value_) {}

constexpr LuaValue::LuaValue() noexcept : inner_value_(LuaNilInnerType{}) {}

LuaValue::ArithmeticOpResult LuaValue::PerformArithmeticOp(
    const LuaValue &rhs, auto operation) const noexcept {
  const auto try_with_float_rhs =
      [&](LuaFloatInnerType Lhs) -> LuaValue::ArithmeticOpResult {
    const auto RhsAsFloat = rhs.TryConvertToFloat();
    if (!RhsAsFloat) {
      return std::unexpected(ArithmeticOpErr::kRhsNotNumeric);
    }

    return LuaValue(LuaFloatInnerType{operation(
        Lhs, std::get<LuaFloatInnerType>(RhsAsFloat.value().inner_value_))});
  };

  const auto visitor = [&](auto &&Value) -> LuaValue::ArithmeticOpResult {
    using T = std::decay_t<decltype(Value)>;

    if constexpr (std::is_same<T, LuaIntInnerType>()) {
      if (rhs.GetValueKind() == LuaValueKind::kInt) {
        return LuaValue(LuaIntInnerType{
            operation(Value, std::get<LuaIntInnerType>(rhs.inner_value_))});
      }

      return try_with_float_rhs(static_cast<LuaFloatInnerType>(Value));
    }

    if constexpr (std::is_same<T, LuaFloatInnerType>()) {
      if (rhs.GetValueKind() == LuaValueKind::kFloat) {
        return LuaValue(LuaFloatInnerType{
            operation(Value, std::get<LuaFloatInnerType>(rhs.inner_value_))});
      }

      return try_with_float_rhs(Value);
    }

    else {
      return std::unexpected(ArithmeticOpErr::kLhsNotNumeric);
    }
  };

  return std::visit(visitor, LuaValue::inner_value_);
}

template <typename InnerT>
inline std::expected<LuaValue, TryMakeErr> LuaValue::TryMakeLuaNum(
    std::string_view Input) {
  InnerT Num{};

  const auto [Ptr, ErrorCode] =
      std::from_chars(Input.data(), Input.data() + Input.length(), Num);

  if (ErrorCode == std::errc{}) {
    return LuaValue(Num);
  }

  if (ErrorCode == std::errc::result_out_of_range) {
    return std::unexpected{TryMakeErr::kOutOfRange};
  }

  return std::unexpected{TryMakeErr::kInvalidInput};
}

inline LuaValueKind LuaValue::GetValueKind() const noexcept {
  const auto visitor = [](auto &&value) -> LuaValueKind {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LuaNilInnerType>()) {
      return LuaValueKind::kNil;
    }

    else if constexpr (std::is_same<T, LuaBoolInnerType>()) {
      return LuaValueKind::kBool;
    }

    else if constexpr (std::is_same<T, LuaIntInnerType>()) {
      return LuaValueKind::kInt;
    }

    else if constexpr (std::is_same<T, LuaFloatInnerType>()) {
      return LuaValueKind::kFloat;
    }

    else if constexpr (std::is_same<T, LuaStringInnerType>()) {
      return LuaValueKind::kString;
    }

    else {
      static_assert(false, "Not every type is covered.");
    }
  };

  return std::visit(visitor, LuaValue::inner_value_);
}

std::string LuaValue::ToString() const noexcept {
  const auto visitor = [](auto &&value) -> std::string {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LuaNilInnerType>()) {
      return "nil";
    }

    else if constexpr (std::is_same<T, LuaBoolInnerType>()) {
      if (value) {
        return "true";
      }

      return "false";
    }

    else if constexpr (std::is_same<T, LuaIntInnerType>() ||
                       std::is_same<T, LuaFloatInnerType>()) {
      return std::to_string(value);
    }

    else if constexpr (std::is_same<T, LuaStringInnerType>()) {
      return std::string{value};
    }

    else {
      static_assert(false, "Not every type is covered.");
    }
  };

  return std::visit(visitor, LuaValue::inner_value_);
}

std::expected<LuaValue, TryMakeErr> LuaValue::TryMakeLuaInteger(
    std::string_view input) noexcept {
  return LuaValue::TryMakeLuaNum<LuaIntInnerType>(input);
}

std::expected<LuaValue, TryMakeErr> LuaValue::TryMakeLuaFloat(
    std::string_view input) noexcept {
  return LuaValue::TryMakeLuaNum<LuaFloatInnerType>(input);
}

std::optional<LuaValue> LuaValue::TryConvertToFloat() const noexcept {
  const auto visitor = [](auto &&value) -> std::optional<LuaValue> {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LuaIntInnerType>()) {
      return LuaValue{static_cast<LuaFloatInnerType>(value)};
    }

    else if constexpr (std::is_same<T, LuaFloatInnerType>()) {
      return LuaValue{value};

    }

    else if constexpr (std::is_same<T, LuaNilInnerType>() ||
                       std::is_same<T, LuaBoolInnerType>() ||
                       std::is_same<T, LuaStringInnerType>()) {
      return std::nullopt;
    }

    else {
      static_assert(false, "Not every type is covered.");
    }
  };

  return std::visit(visitor, LuaValue::inner_value_);
}

LuaValue LuaValue::MakeLuaNil() noexcept { return LuaValue(LuaNilInnerType{}); }

LuaValue LuaValue::MakeLuaString(std::string_view input) noexcept {
  return LuaValue(LuaStringInnerType{input});
}

LuaValue LuaValue::MakeLuaString(std::string &&input) noexcept {
  return LuaValue(LuaStringInnerType{std::move(input)});
}

LuaValue LuaValue::MakeLuaBoolean(bool input) noexcept {
  if (input) {
    return LuaValue(true);
  }

  return LuaValue(false);
}

LuaValue::ArithmeticOpResult LuaValue::operator+(
    const LuaValue &rhs) const noexcept {
  return LuaValue::PerformArithmeticOp(
      rhs, []<typename OperandT>(OperandT lhs, OperandT rhs) -> OperandT {
        return lhs + rhs;
      });
}

LuaValue::ArithmeticOpResult LuaValue::operator-(
    const LuaValue &rhs) const noexcept {
  return LuaValue::PerformArithmeticOp(
      rhs, []<typename OperandT>(OperandT lhs, OperandT rhs) -> OperandT {
        return lhs + rhs;
      });
}

LuaValue::ArithmeticOpResult LuaValue::operator*(
    const LuaValue &rhs) const noexcept {
  return LuaValue::PerformArithmeticOp(
      rhs, []<typename OperandT>(OperandT lhs, OperandT rhs) -> OperandT {
        return lhs + rhs;
      });
}

LuaValue::ArithmeticOpResult LuaValue::operator/(
    const LuaValue &rhs) const noexcept {
  const auto lhs_as_float = LuaValue::TryConvertToFloat();
  if (!lhs_as_float) {
    return std::unexpected(ArithmeticOpErr::kLhsNotNumeric);
  }

  const auto rhs_as_float = rhs.TryConvertToFloat();
  if (!rhs_as_float) {
    return std::unexpected(ArithmeticOpErr::kRhsNotNumeric);
  }

  return LuaValue(LuaFloatInnerType{
      std::get<LuaFloatInnerType>(lhs_as_float.value().inner_value_) /
      std::get<LuaFloatInnerType>(rhs_as_float.value().inner_value_)});
}

LuaValue::ArithmeticOpResult LuaValue::FloorDivide(
    const LuaValue &rhs) const noexcept {
  const auto division_rslt = *this / rhs;
  if (!division_rslt) {
    return division_rslt;
  }

  return LuaValue(LuaFloatInnerType{std::floor(
      std::get<LuaFloatInnerType>(division_rslt.value().inner_value_))});
}

LuaValue::ArithmeticOpResult LuaValue::operator%(
    const LuaValue &rhs) const noexcept {
  const auto division_result = *this % rhs;
  if (!division_result) {
    return division_result;
  }

  return LuaValue(LuaFloatInnerType{std::floor(
      std::get<LuaFloatInnerType>(division_result.value().inner_value_))});
}

LuaValue::ArithmeticOpResult LuaValue::operator^(
    const LuaValue &rhs) const noexcept {
  const auto lhs_as_float = LuaValue::TryConvertToFloat();
  if (!lhs_as_float) {
    return std::unexpected(ArithmeticOpErr::kLhsNotNumeric);
  }

  const auto rhs_as_float = rhs.TryConvertToFloat();
  if (!rhs_as_float) {
    return std::unexpected(ArithmeticOpErr::kRhsNotNumeric);
  }

  return LuaValue(LuaFloatInnerType{std::pow(
      std::get<LuaFloatInnerType>(lhs_as_float.value().inner_value_),
      std::get<LuaFloatInnerType>(rhs_as_float.value().inner_value_))});
}

LuaValue value::operator""_lua_int(unsigned long long int value) {
  return LuaValue(static_cast<LuaIntInnerType>(value));
}

LuaValue value::operator""_lua_float(long double value) {
  return LuaValue(static_cast<LuaFloatInnerType>(value));
}

LuaValue value::operator""_lua_str(const char *string,
                                   std::size_t length) noexcept {
  return LuaValue(LuaStringInnerType(string, length));
}
