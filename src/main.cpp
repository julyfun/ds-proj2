#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "base.hpp"
#include "eval.hpp"
#include "fmt/core.h"
#include "log.hpp"
#include "rust.hpp"
#include "sim.hpp"
#include "strategy.hpp"

// using fmt::logs;
using std::cout;
using std::greater;
using std::make_pair;
using std::map;
using std::optional;
using std::pair;
using std::priority_queue;
using std::set;
using std::string;
using std::vector;

using log::logs;
using log::logs_cargo;

using base::Package;
using base::PackageCategory;
using base::Route;
using base::Station;

using eval::EvalFunc;
using eval::EvalFuncV0;
using eval::EvaluateVersion;

using strategy::dijkstra;
using strategy::dijkstra_enhanced;
using strategy::StrategyVersion;

using sim::Simulation;

// [comptime]

// double get_last_arrival_time(const DataBase& db, string package) {
//     if (db.trips.find(package) == db.trips.end()) {
//         return 0;
//     }
//     double last_time = 0;
//     for (const auto& trip: db.trips.at(package)) {
//         if (trip.type == TripType::ARRIVAL) {
//             last_time = trip.time;
//         }
//     }
//     return last_time;

struct V2StationInfo {
    string id;
    bool try_process_dued;
};

struct V1ACache {
    bool init = false;
};

TEST_CASE("simple") {
    Simulation sim { StrategyVersion::V1, EvaluateVersion::V0 };
    sim.add_station("A", 5, 2, 100);
    sim.add_station("B", 20, 2, 100);
    sim.add_route("A", "B", 100, 50);
    sim.add_route("A", "B", 50, 10);
    sim.add_route("A", "B", 30, 66);
    sim.add_route("A", "B", 200, 10);
    sim.add_order("p1", 100, PackageCategory::STANDARD, "A", "B");
    sim.add_order("p2", 100, PackageCategory::EXPRESS, "A", "B");
    sim.run();
    logs("cost: {}", sim.eval());
}

TEST_CASE("simple-v1b") {
    Simulation sim { StrategyVersion::V1B, EvaluateVersion::V0 };
    sim.add_station("A", 5, 2, 100);
    sim.add_station("B", 20, 2, 100);
    sim.add_route("A", "B", 100, 50);
    sim.add_route("A", "B", 50, 10);
    sim.add_route("A", "B", 30, 66);
    sim.add_route("A", "B", 200, 10);
    sim.add_order("p1", 100, PackageCategory::STANDARD, "A", "B");
    sim.add_order("p2", 100, PackageCategory::EXPRESS, "A", "B");
    sim.run();
    logs("cost: {}", sim.eval());
}

TEST_CASE("smart") {
    Simulation sim { StrategyVersion::V1, EvaluateVersion::V0 };
    sim.add_station("A", 1e3, 0, 0);
    sim.add_station("B", 1, 0, 0);
    sim.add_station("C", 1, 0, 0);
    sim.add_station("D", 1e3, 0, 0);
    sim.add_route("A", "B", 1, 100);
    sim.add_route("A", "C", 1, 100);
    sim.add_route("B", "D", 1, 100);
    sim.add_route("C", "D", 2, 100);
    for (int i = 1; i <= 100; i++) {
        sim.add_order("p" + std::to_string(i), 0, PackageCategory::STANDARD, "A", "D");
    }
    sim.run();
    logs("cost: {}", sim.eval());
}

TEST_CASE("buffer") {
    Simulation sim;
    sim.add_station("a", 10, 4.5, 0);
    sim.add_station("b", 20, 2, 0);
    sim.add_route("a", "b", 100, 1200);
    sim.add_order("p1", 100, PackageCategory::STANDARD, "a", "b");
    sim.add_order("p2", 100, PackageCategory::STANDARD, "a", "b");
    sim.add_order("p3", 100, PackageCategory::STANDARD, "a", "b");
    sim.add_order("p4", 100, PackageCategory::STANDARD, "a", "b");
    sim.add_order("p5", 100, PackageCategory::STANDARD, "a", "b");
    sim.add_order("p6", 100, PackageCategory::STANDARD, "a", "b");
    sim.run();
}

TEST_CASE("main") {
    cout << "Begin\n";
    Simulation sim;
    sim.read_data("../data.txt");
    // sim.schedule_event(new TryProcessOneV1(102, sim, "a"));
    sim.run();
    cout << "End\n";
}
