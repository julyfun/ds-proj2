#ifndef RUST_HPP
#define RUST_HPP

namespace rust {
constexpr double EPS = 1e-9;

bool time_ok(double time, double ok_time) {
    return time > ok_time - EPS;
}
} // namespace rust

#endif
