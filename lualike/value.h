#ifndef LUALIKE_VALUE_H_
#define LUALIKE_VALUE_H_

#include <cstdint>
#include <string>
#include <variant>

namespace lualike::value {

enum class LualikeValueOpErrKind : uint8_t {
  kLhsOperandNotNumeric,
  kRhsOperandNotNumeric,
  kInvalidType,
};

// An exception type to be thrown in operations on LualikeValue. Clarification
// on what happened is given by _error_kind_ member, whose values are
// self-descriptive.
struct LualikeValueOpErr : std::exception {
  LualikeValueOpErrKind error_kind;

  explicit LualikeValueOpErr(LualikeValueOpErrKind error_kind) noexcept;
};

enum class LualikeValueType : uint8_t {
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
struct LualikeValue {
  using NilT = std::monostate;
  using BoolT = bool;
  using IntT = int64_t;
  using FloatT = double;
  using StringT = std::string;

  std::variant<NilT, BoolT, IntT, FloatT, StringT> inner_value;

  explicit constexpr LualikeValue(decltype(inner_value) &&value) noexcept;
  // Construct value such as its type is nil.
  explicit constexpr LualikeValue() noexcept;

  LualikeValueType GetValueType() const noexcept;
  std::string ToString() const noexcept;

  bool operator==(const LualikeValue &rhs) const noexcept = default;

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

constexpr LualikeValue operator""_lua_int(unsigned long long int value);
constexpr LualikeValue operator""_lua_float(long double value);
constexpr LualikeValue operator""_lua_str(const char *string,
                                          std::size_t length) noexcept;

}  // namespace lualike::value

#endif  // LUALIKE_VALUE_H_
