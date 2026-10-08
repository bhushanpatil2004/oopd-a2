#pragma once
// Money: fixed-point currency amount stored in minor units (e.g. paise/cents).
// Using an integer avoids floating-point rounding errors in cost/budget sums.

#include <cstdint>
#include <iosfwd>
#include <string>

namespace bookmgmt {

class Money {
public:
    Money() = default;

    // Construct from minor units: Money::fromMinor(12550) == 125.50
    static Money fromMinor(std::int64_t minor,
                           const std::string& currency = "INR");

    // Construct from major + minor parts: Money::of(125, 50) == 125.50
    static Money of(std::int64_t major,
                    int minor = 0,
                    const std::string& currency = "INR");

    std::int64_t minorUnits() const { return minor_; }
    double toDouble() const { return static_cast<double>(minor_) / 100.0; }
    std::string toString() const;  // "1234.50" / "-3.05"

    const std::string& currency() const { return currency_; }

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(Money o);
    Money& operator-=(Money o);
    Money& operator*=(std::int64_t k) { minor_ *= k; return *this; }

    friend Money operator+(Money a, Money b) { return a += b; }
    friend Money operator-(Money a, Money b) { return a -= b; }
    friend Money operator*(Money a, std::int64_t k) { return a *= k; }
    friend Money operator*(std::int64_t k, Money a) { return a *= k; }

    friend bool operator==(Money a, Money b);
    friend bool operator!=(Money a, Money b);
    friend bool operator<(Money a, Money b);
    friend bool operator<=(Money a, Money b);
    friend bool operator>(Money a, Money b);
    friend bool operator>=(Money a, Money b);

private:
    explicit Money(std::int64_t minor,
                   std::string currency)
        : minor_(minor), currency_(std::move(currency)) {}

    std::int64_t minor_ = 0;
    std::string currency_ = "INR";
};

std::ostream& operator<<(std::ostream& os, Money m);

}  // namespace bookmgmt