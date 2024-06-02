#include <filesystem>
#include <fmt/core.h>
#include <string>

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
} // namespace log
