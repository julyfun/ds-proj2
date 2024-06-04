#ifndef STRATEGY_V2_HPP
#define STRATEGY_V2_HPP

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

namespace strategy::v2 {

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

struct StationPlan {
    // double next_arrival_time;
    StationPlan(string id, Simulation& sim): id(id), sim(sim) {}

    string id;
    Simulation& sim;

    priority_queue<pair<double, string>, vector<pair<double, string>>, greater<>>
        arrival_time_of_due_pkgs;
    // 视为开始 buffer 增加的时间
    double next_arrival_time() const;
    double estimated_wait_time(double now, double arrive_time) const;
    void add_due_pkg(double t, const string& id);
    void pop_due_pkg(double t, const string& id);
};

struct StationInfo {
    optional<double> due_try_time = std::nullopt;
};

// for Simulation to store
struct V2Cache {
    V2Cache() = default;
    explicit V2Cache(Simulation& sim);
    map<string, StationPlan> station_plans;
    map<string, StationInfo> station_info;
};

struct V2Arrival: public Event {
public:
    V2Arrival(double t, Simulation& sim, string package, string dst, bool is_start):
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
struct V2StartSend: public Event {
public:
    V2StartSend(double t, Simulation& sim, string package, string src, int route):
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
struct V2TryProcessOne: public Event {
private:
    string station;

public:
    V2TryProcessOne(double t, Simulation& sim, string station): Event(t, sim), station(station) {}

    void process_event() override;
};

} // namespace strategy::v2
#endif
