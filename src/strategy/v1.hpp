#ifndef STRATEGY_V1_HPP
#define STRATEGY_V1_HPP

#include <string>

#include "event.hpp"
#include "log.hpp"

namespace sim {
struct Simultaion;
}

namespace strategy::v1 {
using std::string;

using event::Event;
using log::logs;
using log::logs_cargo;
using sim::Simulation;

struct V1Arrival: public Event {
public:
    V1Arrival(double t, Simulation& sim, string package, string dst):
        Event(t, sim),
        package(package),
        station(dst) {}

    void process_event() override;

private:
    string package;
    string station;
};

struct V0StartProcess: public Event {
public:
    [[deprecated("V0 is not used anymore")]]
    V0StartProcess(double t, Simulation& sim, string package, string src, int route):
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

// [可能是送原地，即完成配送]
// EndProcess
struct V1StartSend: public Event {
public:
    V1StartSend(double t, Simulation& sim, string package, string src, int route):
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
struct V1TryProcessOne: public Event {
private:
    string station;

public:
    V1TryProcessOne(double t, Simulation& sim, string station): Event(t, sim), station(station) {}

    void process_event() override;
};

} // namespace strategy::v1
#endif
