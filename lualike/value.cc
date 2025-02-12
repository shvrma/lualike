#include "value.h"

#include <charconv>
#include <string>
#include <utility>

namespace lualike::value {

constexpr LuaValue::LuaValue(decltype(value_) &&value) noexcept
    : value_(value) {}

LuaValue::LuaValue() noexcept : value_(LuaNilInnerType{}) {}

inline LuaValueType LuaValue::GetValueType() const noexcept {
  const auto visitor = [](auto &&value) -> LuaValueType {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LuaNilInnerType>()) {
      return LuaValueType::kNil;
    }

    else if constexpr (std::is_same<T, LuaBoolInnerType>()) {
      return LuaValueType::kBoolean;
    }

    else if constexpr (std::is_same<T, LuaIntInnerType>()) {
      return LuaValueType::kInteger;
    }

    else if constexpr (std::is_same<T, LuaFloatInnerType>()) {
      return LuaValueType::kFloat;
    }

    else if constexpr (std::is_same<T, LuaStringInnerType>()) {
      return LuaValueType::kString;
    }

    else {
      static_assert(false, "Not every type is covered.");
    }
  };

  return std::visit(visitor, LuaValue::value_);
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

  return std::visit(visitor, LuaValue::value_);
}

LuaValue::TryMakeResult LuaValue::TryMakeLuaInteger(
    std::string_view input) noexcept {
  LuaIntInnerType num{};

  const auto [ptr, error_code] =
      std::from_chars(input.data(), input.data() + input.length(), num);

  if (error_code == std::errc{}) {
    return LuaValue(num);
  }

  if (error_code == std::errc::invalid_argument) {
    return LuaValue::TryMakeErr::kInvalidInput;
  }

  if (error_code == std::errc::result_out_of_range) {
    return LuaValue::TryMakeErr::kOutOfRange;
  }

  return LuaValue::TryMakeErr::kInvalidInput;
}

LuaValue::TryMakeResult LuaValue::TryMakeLuaFloat(
    std::string_view input) noexcept {
  LuaFloatInnerType num{};

  const auto [ptr, error_code] =
      std::from_chars(input.data(), input.data() + input.length(), num);

  if (error_code == std::errc{}) {
    return LuaValue(num);
  }

  if (error_code == std::errc::invalid_argument) {
    return LuaValue::TryMakeErr::kInvalidInput;
  }

  if (error_code == std::errc::result_out_of_range) {
    return LuaValue::TryMakeErr::kOutOfRange;
  }

  return LuaValue::TryMakeErr::kInvalidInput;
}

LuaValue LuaValue::MakeLuaNil() noexcept { return LuaValue(LuaNilInnerType{}); }

LuaValue LuaValue::MakeLuaString(std::string_view input) noexcept {
  return LuaValue(LuaStringInnerType{input});
}

LuaValue LuaValue::MakeLuaString(std::string &&input) noexcept {
  return LuaValue(std::move(input));
}

LuaValue LuaValue::MakeLuaBoolean(bool input) noexcept {
  if (input) {
    return LuaValue(true);
  }

  return LuaValue(false);
}

LuaValue LuaValue::operator+(const LuaValue &rhs) const noexcept {
  const auto visitor = [&rhs](auto &&value) -> LuaValue {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LuaIntInnerType>()) {
      if (rhs.GetValueType() != LuaValueType::kInteger) {
        return LuaValue{};
      }

      return LuaValue(value + std::get<LuaIntInnerType>(rhs.value_));
    }

    else {
      // static_assert(false, "Not every type is covered.");
      return LuaValue{};
    }
  };

  return std::visit(visitor, LuaValue::value_);
}

LuaValue LuaValue::operator-(const LuaValue &rhs) const noexcept {
  const auto visitor = [&rhs](auto &&value) -> LuaValue {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LuaIntInnerType>()) {
      if (rhs.GetValueType() != LuaValueType::kInteger) {
        return LuaValue{};
      }

      return LuaValue(value - std::get<LuaIntInnerType>(rhs.value_));
    }

    else {
      // static_assert(false, "Not every type is covered.");
      return LuaValue{};
    }
  };

  return std::visit(visitor, LuaValue::value_);
}

LuaValue LuaValue::operator*(const LuaValue &rhs) const noexcept {
  const auto visitor = [&rhs](auto &&value) -> LuaValue {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LuaIntInnerType>()) {
      if (rhs.GetValueType() != LuaValueType::kInteger) {
        return LuaValue{};
      }

      return LuaValue(value * std::get<LuaIntInnerType>(rhs.value_));
    }

    else {
      // static_assert(false, "Not every type is covered.");
      return LuaValue{};
    }
  };

  return std::visit(visitor, LuaValue::value_);
}

LuaValue LuaValue::operator/(const LuaValue &rhs) const noexcept {
  const auto visitor = [&rhs](auto &&value) -> LuaValue {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same<T, LuaIntInnerType>()) {
      if (rhs.GetValueType() != LuaValueType::kInteger) {
        return LuaValue{};
      }

      return LuaValue(value / std::get<LuaIntInnerType>(rhs.value_));
    }

    else {
      // static_assert(false, "Not every type is covered.");
      return LuaValue{};
    }
  };

  return std::visit(visitor, LuaValue::value_);
}

LuaValue LuaValue::operator+=(const LuaValue &rhs) noexcept {
  LuaValue::value_ = (*this + rhs).value_;
  return *this;
}

LuaValue LuaValue::operator-=(const LuaValue &rhs) noexcept {
  LuaValue::value_ = (*this - rhs).value_;
  return *this;
}

LuaValue LuaValue::operator*=(const LuaValue &rhs) noexcept {
  LuaValue::value_ = (*this * rhs).value_;
  return *this;
}

LuaValue LuaValue::operator/=(const LuaValue &rhs) noexcept {
  LuaValue::value_ = (*this / rhs).value_;
  return *this;
}

LuaValue operator""_lua_int(unsigned long long int value) {
  return LuaValue(static_cast<LuaValue::LuaIntInnerType>(value));
}

LuaValue operator""_lua_float(long double value) {
  return LuaValue(static_cast<LuaValue::LuaFloatInnerType>(value));
}

LuaValue operator""_lua_str(const char *str, std::size_t len) noexcept {
  return LuaValue::MakeLuaString(std::string(str, len));
}

void PrintTo(const LuaValue &value, std::ostream *output) {
  *output << value.ToString();
}

}  // namespace lualike::value
