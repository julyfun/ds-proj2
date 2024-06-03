#ifndef RUST_HPP
#define RUST_HPP

#include <cmath>

namespace rust {
constexpr double EPS = 1e-9;

inline bool time_ok(double time, double ok_time) {
    return time > ok_time - EPS;
}

inline bool eq(double a, double b) {
    return std::abs(a - b) < EPS;
}

} // namespace rust

#endif
