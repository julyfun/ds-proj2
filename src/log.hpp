#ifndef LOG_HPP
#define LOG_HPP

#include <filesystem>
#include <string>

#include "fmt/color.h"
#include "fmt/core.h"

namespace log {
using fmt::println;
using std::string;
using std::filesystem::path;

template<typename... T>
void logs(const std::string_view& str, T&&... args) {
    // std::filesystem::path file_path("log.txt");
    // auto* file = fopen(file_path.string().c_str(), "w");
    auto* file = stdout;
    // fmt::println(file, fmt::runtime(str), std::forward<T>(args)...);
    fmt::println(fmt::runtime(str), std::forward<T>(args)...);
    // fclose(file); // Better use RAII instead.
}

template<typename... T>
void logs_info(const std::string_view& str, T&&... args) {
    // std::filesystem::path file_path("log.txt");
    // auto* file = fopen(file_path.string().c_str(), "w");
    auto* file = stdout;
    std::string msg =
        fmt::format(fg(fmt::rgb(0xFFFF00)) | fmt::emphasis::bold, str, std::forward<T>(args)...);
    // fmt::println(file, fmt::runtime(str), std::forward<T>(args)...);
    fmt::println(msg);
    // fclose(file); // Better use RAII instead.
}

template<typename... T>
void logs_cargo(const std::string_view& cargo, const std::string_view& msg, T&&... args) {
    // std::filesystem::path file_path("log.txt");
    // auto* file = fopen(file_path.string().c_str(), "w");
    auto* file = stdout;
    std::string cargo_str =
        fmt::format(fg(fmt::rgb(0xADE099)) | fmt::emphasis::bold, fmt::format("{:>12}", cargo));
    // fmt::println(file, fmt::runtime(str), std::forward<T>(args)...);
    std::string msg_str = fmt::format(msg, std::forward<T>(args)...);
    fmt::println("{} {}", cargo_str, msg_str);
    // fclose(file); // Better use RAII instead.
}

} // namespace log

#endif
