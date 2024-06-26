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

TEST_CASE("smart-pk") {
    // for (int i = 1; i <= 5; i++) {
    //     Simulation sim { [&i]() {
    //                         if (i == 1) {
    //                             return StrategyVersion::V1;
    //                         } else if (i == 2) {
    //                             return StrategyVersion::V1B;
    //                         } else if (i == 3) {
    //                             return StrategyVersion::V2;
    //                         } else if (i == 4) {
    //                             return StrategyVersion::V2B;
    //                         } else {
    //                             return StrategyVersion::V3;
    //                         }
    //                     }(),
    //                      EvaluateVersion::V1 };
    // }
    const string snames[] = { "V1", "V1B", "V2", "V2B", "V3" };
    const string enames[] = { "V0", "V1" };
    for (int eva = 0; eva < 2; eva++) {
        log::ecargo("Eval " + enames[eva], "====================");
        for (int stg = 0; stg < 5; stg++) {
            auto stg_ver = [&stg]() {
                if (stg == 0) {
                    return StrategyVersion::V1;
                } else if (stg == 1) {
                    return StrategyVersion::V1B;
                } else if (stg == 2) {
                    return StrategyVersion::V2;
                } else if (stg == 3) {
                    return StrategyVersion::V2B;
                } else {
                    return StrategyVersion::V3;
                }
            }();
            auto eva_ver = [&eva]() {
                if (eva == 0) {
                    return EvaluateVersion::V0;
                } else {
                    return EvaluateVersion::V1;
                }
            }();
            Simulation sim { stg_ver, eva_ver };
            sim.add_station("A", 1e3, 0, 0);
            sim.add_station("B", 1, 0, 0);
            sim.add_station("C", 1, 0, 0);
            sim.add_station("D", 1e3, 0, 0);
            sim.add_route("A", "B", 1, 1);
            sim.add_route("A", "C", 1, 1);
            sim.add_route("B", "D", 1, 1);
            sim.add_route("C", "D", 1.01, 1);
            for (int i = 1; i <= 100; i++) {
                sim.add_order(
                    "p" + std::to_string(i),
                    0.001 * i,
                    i <= 50 ? PackageCategory::STANDARD : PackageCategory::EXPRESS,
                    "A",
                    "D"
                );
            }
            sim.run();
            log::ecargo(snames[stg], "cost: {} events: {}", sim.eval(), sim.event_cnt);
        }
    }
}

TEST_CASE("main-pk") {
    const string snames[] = { "V1", "V1B", "V2", "V2B", "V3" };
    const string enames[] = { "V0", "V1" };
    for (int eva = 0; eva < 2; eva++) {
        log::ecargo("Eval " + enames[eva], "====================");
        for (int stg = 0; stg < 5; stg++) {
            auto stg_ver = [&stg]() {
                if (stg == 0) {
                    return StrategyVersion::V1;
                } else if (stg == 1) {
                    return StrategyVersion::V1B;
                } else if (stg == 2) {
                    return StrategyVersion::V2;
                } else if (stg == 3) {
                    return StrategyVersion::V2B;
                } else {
                    return StrategyVersion::V3;
                }
            }();
            auto eva_ver = [&eva]() {
                if (eva == 0) {
                    return EvaluateVersion::V0;
                } else {
                    return EvaluateVersion::V1;
                }
            }();
            Simulation sim { stg_ver, eva_ver };
            sim.read_data("../data/data.txt");
            // sim.schedule_event(new TryProcessOneV1(102, sim, "a"));
            sim.run();
            log::ecargo(snames[stg], "cost: {} events: {}", sim.eval(), sim.event_cnt);
        }
    }
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

TEST_CASE("main-v1") {
    Simulation sim { StrategyVersion::V1B, EvaluateVersion::V0 };
    sim.read_data("../data.txt");
    // sim.schedule_event(new TryProcessOneV1(102, sim, "a"));
    sim.run();
    log::ecargo("v1-main", "cost: {}", sim.eval());
    log::ecargo("Tag", "events: {}", sim.event_cnt);
}

TEST_CASE("main-v2") {
    Simulation sim { StrategyVersion::V2B, EvaluateVersion::V0 };
    sim.read_data("../data.txt");
    // sim.schedule_event(new TryProcessOneV1(102, sim, "a"));
    sim.run();
    log::ecargo("v2-main", "cost: {}", sim.eval());
    log::ecargo("Tag", "events: {}", sim.event_cnt);
    // print all plans' size' in v2_cache
    // for (auto& [key, value]: sim.v2_cache.station_plans) {
    //     CHECK(value.arrival_time_of_due_pkgs.size() == 0);
    // }
}

TEST_CASE("main-v3") {
    Simulation sim { StrategyVersion::V3, EvaluateVersion::V1 };
    sim.read_data("../data/data.txt");
    // sim.schedule_event(new TryProcessOneV1(102, sim, "a"));
    sim.run();
    log::ecargo("v3-main", "cost: {}", sim.eval());
    log::ecargo("Tag", "events: {}", sim.event_cnt);
    // print all plans' size' in v2_cache
    // for (auto& [key, value]: sim.v2_cache.station_plans) {
    //     CHECK(value.arrival_time_of_due_pkgs.size() == 0);
    // }
}

TEST_CASE("simple-v2") {
    Simulation sim { StrategyVersion::V2, EvaluateVersion::V0 };
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

TEST_CASE("simple-v3") {
    Simulation sim { StrategyVersion::V3, EvaluateVersion::V0 };
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
