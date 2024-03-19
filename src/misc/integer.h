#pragma once
#include "../common.h"

namespace salt {

// Arbitrary precision integer
class Integer {
private:
    // represented in reverse for efficiency
    std::string str_;

    // remove leading zeroes from str
    void update();
public:
    operator int() const;
    explicit operator long long() const;
    Integer operator-() const;

    Integer operator+(const Integer& right) const;
    Integer operator-(const Integer& right) const;
    Integer operator*(const Integer& right) const;
    Integer operator/(const Integer& right) const;
    bool operator<(const Integer& right) const;
    bool operator>(const Integer& right) const;
    bool operator==(const Integer& right) const;
    const std::string& str() const { return str_; }
    friend std::ostream& operator<<(std::ostream& os, const Integer& integer);
    inline bool is_negative() const { return str_[str_.size() - 1] == '-'; }
    inline bool is_positive() const { return str_[str_.size() - 1] != '-'; }



    // relying heavily on type promotion...
    Integer(long long n);
    explicit Integer(unsigned long long n); // explicit so the "default" implicit constructor for numbers is Integer(long long n) 
    Integer(const std::string& s);
    Integer(const std::string&& s);
    Integer() : str_("0") {}
};


long long atoll(const Integer& integer);
int atoi(const Integer& integer);

}


salt::Integer abs(const salt::Integer i);