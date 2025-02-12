#ifndef LUALIKE_VALUE_H_
#define LUALIKE_VALUE_H_

#include <cstdint>
#include <ostream>
#include <string>
#include <variant>

namespace lualike::value {

enum class LuaValueType : uint8_t {
  kNil,
  kBoolean,
  kInteger,
  kFloat,
  kString,
  // kFunction,
  // kUserdata,
  // kThread,
  // kTable,
};

class LuaValue {
  using LuaNilInnerType = std::monostate;
  using LuaBoolInnerType = bool;
  using LuaIntInnerType = int64_t;
  using LuaFloatInnerType = double;
  using LuaStringInnerType = std::string;

  std::variant<LuaNilInnerType, LuaBoolInnerType, LuaIntInnerType,
               LuaFloatInnerType, LuaStringInnerType>
      value_;

  explicit constexpr LuaValue(decltype(value_) &&value) noexcept;

 public:
  explicit LuaValue() noexcept;

  LuaValueType GetValueType() const noexcept;
  std::string ToString() const noexcept;

  enum class TryMakeErr : uint8_t {
    kOutOfRange,
    kInvalidInput,
  };
  using TryMakeResult = std::variant<LuaValue, TryMakeErr>;
  static TryMakeResult TryMakeLuaInteger(std::string_view input) noexcept;
  static TryMakeResult TryMakeLuaFloat(std::string_view input) noexcept;

  static LuaValue MakeLuaNil() noexcept;
  static LuaValue MakeLuaString(std::string_view input) noexcept;
  static LuaValue MakeLuaString(std::string &&input) noexcept;
  static LuaValue MakeLuaBoolean(bool input) noexcept;

  bool operator==(const LuaValue &rhs) const noexcept = default;

  LuaValue operator+(const LuaValue &rhs) const noexcept;
  LuaValue operator-(const LuaValue &rhs) const noexcept;
  LuaValue operator/(const LuaValue &rhs) const noexcept;
  LuaValue operator*(const LuaValue &rhs) const noexcept;

  LuaValue operator+=(const LuaValue &rhs) noexcept;
  LuaValue operator-=(const LuaValue &rhs) noexcept;
  LuaValue operator/=(const LuaValue &rhs) noexcept;
  LuaValue operator*=(const LuaValue &rhs) noexcept;

  friend LuaValue operator""_lua_int(unsigned long long int value);
  friend LuaValue operator""_lua_float(long double value);
  friend LuaValue operator""_lua_str(const char *str, std::size_t len) noexcept;

  friend void PrintTo(const LuaValue &value, std::ostream *output);
};

LuaValue operator""_lua_int(unsigned long long int value);
LuaValue operator""_lua_float(long double value);
LuaValue operator""_lua_str(const char *str, std::size_t len) noexcept;

}  // namespace lualike::value

#endif  // LUALIKE_VALUE_H_
