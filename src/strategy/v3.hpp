#ifndef STRATEGY_V3_HPP
#define STRATEGY_V3_HPP

#include <limits>
#include <map>
#include <queue>
#include <set>
#include <sstream>

#include "event.hpp"
#include "log.hpp"
#include "rust.hpp"

namespace sim {
struct Simulation;
}

namespace strategy::v3 {

using sim::Simulation;
using std::greater;
using std::map;
using std::multiset;
using std::optional;
using std::pair;
using std::priority_queue;
using std::string;
using std::vector;

using event::Event;
using log::logs;
using log::logs_cargo;

struct V3Arrival: public Event {
public:
    V3Arrival(double t, Simulation& sim, string package, string dst, bool is_start):
        Event(t, sim),
        package(package),
        station(dst),
        is_start(is_start) {}

    void process_event() override;

private:
    string package;
    string station;
    bool is_start;
};

// [可能是送原地，即完成配送]
// EndProcess
struct V3StartSend: public Event {
public:
    V3StartSend(double t, Simulation& sim, string package, string src, int route):
        Event(t, sim),
        package(package),
        src(src),
        route(route) {}

    void process_event() override;

private:
    string package;
    string src;
    int route;
};

// 检查 buffer 和从 buffer 中拿出内容 buffer 必须在同一个 event
struct V3TryProcessOne: public Event {
private:
    string station;

public:
    V3TryProcessOne(double t, Simulation& sim, string station): Event(t, sim), station(station) {}

    void process_event() override;
};

} // namespace strategy::v3
#endif
