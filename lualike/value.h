#ifndef LUALIKE_VALUE_H_
#define LUALIKE_VALUE_H_

#include <compare>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace lualike::value {

enum class LualikeValueOpErrKind : uint8_t {
  kNotNumericOperand,
  kNotBoolOperand,
  kNormallyImpossibleErr,
  kLhsNotNumeric,
  kLhsNotBool,
  kRhsNotNumeric,
  kRhsNotBool,
};

struct LualikeValueOpErr : std::exception {
  LualikeValueOpErrKind error_kind;

  explicit LualikeValueOpErr(LualikeValueOpErrKind error_kind) noexcept
      : error_kind(error_kind) {}
};

struct LualikeValue;

struct LualikeFunction {
  std::vector<std::string> args;

  explicit LualikeFunction(std::vector<std::string>&& func_args) noexcept
      : args(std::move(func_args)) {}

  virtual ~LualikeFunction() = default;

  virtual std::optional<LualikeValue> Call(std::vector<LualikeValue> args) = 0;
};

struct LualikeValue {
  using NilT = std::monostate;
  using BoolT = bool;
  using IntT = int64_t;
  using FloatT = double;
  using StringT = std::string;
  using FuncT = std::shared_ptr<LualikeFunction>;

  std::variant<NilT, BoolT, IntT, FloatT, StringT, FuncT> inner_value{NilT{}};

  friend bool operator==(const LualikeValue&, const LualikeValue&) = default;
  friend auto operator<=>(const LualikeValue&, const LualikeValue&) = default;

  std::string ToString() const;

  LualikeValue& operator+=(const LualikeValue& rhs);
  LualikeValue& operator-=(const LualikeValue& rhs);
  LualikeValue& operator*=(const LualikeValue& rhs);
  LualikeValue& operator/=(const LualikeValue& rhs);
  LualikeValue& operator%=(const LualikeValue& rhs);
  LualikeValue& ExponentiateAndAssign(const LualikeValue& rhs);
  LualikeValue& FloorDivideAndAssign(const LualikeValue& rhs);

  friend LualikeValue operator+(LualikeValue lhs, const LualikeValue& rhs);
  friend LualikeValue operator-(LualikeValue lhs, const LualikeValue& rhs);
  friend LualikeValue operator*(LualikeValue lhs, const LualikeValue& rhs);
  friend LualikeValue operator/(LualikeValue lhs, const LualikeValue& rhs);
  friend LualikeValue operator%(LualikeValue lhs, const LualikeValue& rhs);
  friend LualikeValue Exponentiate(LualikeValue lhs, const LualikeValue& rhs);
  friend LualikeValue FloorDivide(LualikeValue lhs, const LualikeValue& rhs);

  friend LualikeValue operator-(const LualikeValue& operand);
  friend LualikeValue operator||(const LualikeValue& lhs,
                                 const LualikeValue& rhs);
  friend LualikeValue operator&&(const LualikeValue& lhs,
                                 const LualikeValue& rhs);
  friend LualikeValue operator!(const LualikeValue& operand);
};

void PrintTo(const LualikeValue& value, std::ostream* os);

}  // namespace lualike::value

#endif  // LUALIKE_VALUE_H_
