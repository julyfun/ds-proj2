#ifndef SIM_HPP
#define SIM_HPP

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "fmt/core.h"

#include "base.hpp"
#include "eval.hpp"
#include "event.hpp"
#include "log.hpp"
#include "rust.hpp"
#include "strategy.hpp"
#include "strategy/v1.hpp"
#include "strategy/v2.hpp"
#include "strategy/v3.hpp"
// #include "strategy/v2.hpp"

namespace sim {

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

using event::Event;
using event::EventComparator;

using log::logs;
using log::logs_cargo;

using base::Package;
using base::PackageCategory;
using base::Route;
using base::Station;

using eval::EvalFunc;
using eval::EvalFuncV0;
using eval::EVALUATE_FUNC_MAP;
using eval::EvaluateVersion;

using strategy::dijkstra;
using strategy::dijkstra_enhanced;
using strategy::StrategyVersion;

struct Simulation {
private:
    double current_time; // current time
    std::priority_queue<Event*, std::vector<Event*, std::allocator<Event*>>, EventComparator>
        event_queue;
    int arrived = 0;
    int route_cnt = 0;
    double transport_cost = 0;

public:
    int event_cnt = 0;

public:
    map<string, Station> stations;
    map<string, map<int, Route>> routes;
    map<string, Package> packages;

    // map<string, map<string,
public:
    const StrategyVersion strategy_version = StrategyVersion::V1;
    const EvaluateVersion evaluate_version = EvaluateVersion::V0;

public:
    // strategy info
    // any other ways?
    // map<string, V2StationInfo> v2_station_info;
    strategy::v2::V2Cache v2_cache;

public:
    Simulation() = default;
    Simulation(StrategyVersion strategy_version, EvaluateVersion evaluate_version):
        strategy_version(strategy_version),
        evaluate_version(evaluate_version) {}

    void run();

    void schedule_event(Event* event) {
        this->event_queue.push(event);
    }

    void
    add_order(string id, double time, PackageCategory ctg, string src, string dst); // add station

    void add_transport_cost(double cost) {
        this->transport_cost += cost;
    }

    void add_station(string id, double throughput, double process_delay, double cost) {
        this->stations[id] = Station { id, throughput, process_delay, cost };
        this->routes.emplace(id, map<int, Route>());

        if (this->strategy_version == StrategyVersion::V2
            || this->strategy_version == StrategyVersion::V2B
            || this->strategy_version == StrategyVersion::V3)
        {
            this->v2_cache.station_plans.emplace(id, strategy::v2::StationPlan(id, *this));
            this->v2_cache.station_info.emplace(id, strategy::v2::StationInfo());
        }
    }
    // add route
    void add_route(string src, string dst, double time, double cost) {
        // check src and dst exist
        assert(this->stations.find(src) != this->stations.end());
        assert(this->stations.find(dst) != this->stations.end());
        this->route_cnt += 1;
        auto route = Route { this->route_cnt, src, dst, time, cost };
        this->routes.at(src).emplace(this->route_cnt, route); // add key value
    }
    // arrive package
    void finish_order(string package, double time) {
        // finish log
        logs(
            "package {} arrived at {}, spent {}",
            package,
            time,
            time - this->packages[package].time_created
        );
        this->arrived += 1;
        this->packages[package].finished = true;
        this->packages[package].time_finished = time;
    }

    // fuck c++
    double eval() {
        return (*(EVALUATE_FUNC_MAP[static_cast<int>(this->evaluate_version)].second))(
            this->transport_cost,
            this->packages
        );
    }

    void read_data(const string& path) {
        std::ifstream file(path, std::ios::in);
        if (file.is_open()) {
            string line;
            bool is_stations_section = false;
            bool is_routes_section = false;
            bool is_orders_section = false;
            while (std::getline(file, line)) {
                if (line == "stations:") {
                    is_stations_section = true;
                    is_routes_section = false;
                    is_orders_section = false;
                } else if (line == "edges:") {
                    is_stations_section = false;
                    is_routes_section = true;
                    is_orders_section = false;
                } else if (line == "packets:") {
                    is_stations_section = false;
                    is_routes_section = false;
                    is_orders_section = true;
                } else if (is_stations_section) {
                    std::stringstream ss(line);
                    string id;
                    double throughput, time_process, cost;
                    char p_l; // parenthesis left
                    char c; // comma
                    char p_r; // parenthesis right

                    ss >> id >> c >> p_l >> throughput >> c >> time_process >> c >> cost >> p_r;
                    this->add_station(id, throughput, time_process, cost);
                } else if (is_routes_section) {
                    std::stringstream ss(line);
                    string src;
                    string dst;
                    double time_cost, money_cost;
                    char c;

                    ss >> src >> c >> dst >> c >> time_cost >> c >> money_cost;
                    this->add_route(src, dst, time_cost, money_cost);
                } else if (is_orders_section) {
                    std::stringstream ss(line);
                    double time;
                    int ctg;
                    string src;
                    string dst;
                    string id;
                    char c;

                    ss >> id >> c >> time >> c >> ctg >> c >> src >> c >> dst;
                    if (ctg == 0)
                        this->add_order(id, time, PackageCategory::STANDARD, src, dst);
                    else
                        this->add_order(id, time, PackageCategory::EXPRESS, src, dst);
                }
            }
        } else {
            std::cout << "File not found" << std::endl;
        }
    }
};

} // namespace sim

#endif
