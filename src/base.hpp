#ifndef BASE_HPP
#define BASE_HPP

#include <set>
#include <string>

namespace base {
// minimal
using std::set;
using std::string;

struct Route {
    int id;
    string src;
    string dst;
    double time;
    double cost;
};

struct Station {
public:
    string id;
    double throughput;
    double process_delay;

    set<string> buffer;
    // 下一个可以开始处理的时间
    double start_process_ok_time = std::numeric_limits<double>::min();
    // optional<string> processing_package;

public:
    void take_package_from_buffer_to_processing(string package, double time) {
        this->buffer.erase(package);
        this->start_process_ok_time = time + 1.0 / this->throughput;
        // this->processing_package = package;
    }
};

enum struct PackageCategory {
    STANDARD,
    EXPRESS,
};

struct Package {
    string id;
    PackageCategory category;
    double time_created;
    string src;
    string dst;
    // dynamic
    bool finished;
    double time_finished;
};

} // namespace base
#endif
