#ifndef LUALIKE_VALUE_H_
#define LUALIKE_VALUE_H_

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <variant>

namespace lualike::value {

// Specifies the type of a value stored in LuaValue type
enum class LuaValueKind : uint8_t {
  kNil,
  kBool,
  kInt,
  kFloat,
  kString,
};

using LuaNilInnerType = std::monostate;
using LuaBoolInnerType = bool;
using LuaIntInnerType = int64_t;
using LuaFloatInnerType = double;
using LuaStringInnerType = std::string;

enum class ArithmeticOpErr : uint8_t { kLhsNotNumeric, kRhsNotNumeric };

enum class TryMakeErr : uint8_t {
  kOutOfRange,
  kInvalidInput,
};

// Represents a single value within lualike
//
// Note: there are no other types for this purpose as it stores value by itself,
// not looking at its data type. You can check the kind of a VALUE inside by
// calling GetValueKind().
class LuaValue {
  std::variant<LuaNilInnerType, LuaBoolInnerType, LuaIntInnerType,
               LuaFloatInnerType, LuaStringInnerType>
      inner_value_;

  explicit constexpr LuaValue(decltype(inner_value_) &&value) noexcept;
  explicit constexpr LuaValue() noexcept;

  using ArithmeticOpResult = std::expected<LuaValue, ArithmeticOpErr>;
  using TryMakeResult = std::expected<LuaValue, TryMakeErr>;

  ArithmeticOpResult PerformArithmeticOp(const LuaValue &rhs,
                                         auto operation) const noexcept;

  template <typename InnerT>
  static TryMakeResult TryMakeLuaNum(std::string_view input);

 public:
  LuaValueKind GetValueKind() const noexcept;
  std::string ToString() const noexcept;

  static TryMakeResult TryMakeLuaInteger(std::string_view input) noexcept;
  static TryMakeResult TryMakeLuaFloat(std::string_view input) noexcept;
  std::optional<LuaValue> TryConvertToFloat() const noexcept;

  static LuaValue MakeLuaNil() noexcept;
  static LuaValue MakeLuaString(std::string_view input) noexcept;
  static LuaValue MakeLuaString(std::string &&input) noexcept;
  static LuaValue MakeLuaBoolean(bool input) noexcept;

  bool operator==(const LuaValue &rhs) const noexcept = default;

  ArithmeticOpResult operator+(const LuaValue &rhs) const noexcept;
  ArithmeticOpResult operator-(const LuaValue &rhs) const noexcept;
  ArithmeticOpResult operator*(const LuaValue &rhs) const noexcept;
  ArithmeticOpResult operator/(const LuaValue &rhs) const noexcept;
  ArithmeticOpResult FloorDivide(const LuaValue &rhs) const noexcept;
  ArithmeticOpResult operator%(const LuaValue &rhs) const noexcept;
  // Not a binary XOR but exponantiation operation
  ArithmeticOpResult operator^(const LuaValue &rhs) const noexcept;

  friend LuaValue operator""_lua_int(unsigned long long int value);
  friend LuaValue operator""_lua_float(long double value);
  friend LuaValue operator""_lua_str(const char *string,
                                     std::size_t length) noexcept;
};

}  // namespace lualike::value

#endif  // LUALIKE_VALUE_H_
