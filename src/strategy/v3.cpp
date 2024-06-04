#include "strategy/v3.hpp"

#include "base.hpp"
#include "eval.hpp"
#include "log.hpp"
#include "sim.hpp"
#include "strategy.hpp"
#include "strategy/v2.hpp"

namespace strategy::v3 {

using std::make_pair;
using strategy::v2::StationPlan;

void try_due_try(double t, Simulation& sim, string station) {
    // check cached due time
    if (!sim.v2_cache.station_info.at(station).due_try_time.has_value()) {
        sim.v2_cache.station_info.at(station).due_try_time = t;
        sim.schedule_event(new V3TryProcessOne(t, sim, station));
    }
    if (t < sim.v2_cache.station_info.at(station).due_try_time) {
        sim.v2_cache.station_info.at(station).due_try_time = t;
        sim.schedule_event(new V3TryProcessOne(t, sim, station));
        return;
    }
}

struct DijRes {
    map<string, pair<string, int>> prev;
    map<string, double> start_send_time_at_min_cost;
};

DijRes dijkstra_tree(
    const map<string, Station>& stations,
    const map<string, map<int, Route>>& routes,
    string src,
    double start_process_time,
    const map<string, StationPlan>& station_plans,
    double money_coefficient = 1.0,
    double time_coefficient = 1.667
) {
    logs_cargo("Info", "legend_dijkstra called");
    map<string, double> cost;
    map<string, double> start_send_time_at_min_cost;
    map<string, pair<string, int>> prev; // nodes' prev station and route
    for (const auto& [id, station]: stations) {
        cost[id] = std::numeric_limits<double>::max();
        start_send_time_at_min_cost[id] = std::numeric_limits<double>::max();
    }
    cost[src] = 0;
    start_send_time_at_min_cost[src] = start_process_time + stations.at(src).process_delay;
    // priority_queue<pair<double, string>> q;
    priority_queue<pair<double, string>, vector<pair<double, string>>, greater<>> q;

    q.push(make_pair(0, src));
    while (!q.empty()) {
        auto [d, u] = q.top();
        q.pop();
        if (d > cost[u]) {
            continue;
        }
        // if not found, edges is empty
        auto edges = routes.find(u) == routes.end() ? map<int, Route> {} : routes.at(u);
        for (const auto& route: edges) {
            string v = route.second.dst;
            const double estimated_wait_time = station_plans.at(v).estimated_wait_time(
                start_send_time_at_min_cost.at(u),
                start_send_time_at_min_cost.at(u) + route.second.time
            );
            // logs_cargo("Info", "{}", estimated_wait_time);
            double time_gonna_be_spent =
                route.second.time + estimated_wait_time + stations.at(v).process_delay;
            const double w = time_gonna_be_spent * time_coefficient
                + (route.second.cost + stations.at(v).cost) * money_coefficient;
            if (cost[u] + w < cost[v]) {
                cost[v] = cost[u] + w;
                start_send_time_at_min_cost[v] =
                    start_send_time_at_min_cost.at(u) + time_gonna_be_spent;
                prev[v] = { u, route.first };
                q.push(make_pair(cost[v], v));
            }
        }
    }
    // print prevs

    // vector<int> path;
    // for (string at = dst; at != ""; at = prev[at]) {
    //     path.push_back(at);
    // }
    // for (string at = dst; at != src;) {
    //     auto [from, route] = prev[at];
    //     // logs("from: {}, route: {}", from, route);
    //     path.push_back(route);
    //     at = from;
    // }
    // std::reverse(path.begin(), path.end());
    return { prev, start_send_time_at_min_cost };
}

void V3TryProcessOne::process_event() {
    // std::ofstream file("output.txt", std::ios::app);
    // std::ofstream file("output.txt", std::ios::app);
    // if is not dued
    if (!this->sim.v2_cache.station_info[this->station].due_try_time.has_value()) {
        logs_cargo("Error", "station {} try to process one but no due time.");
        return;
    }
    if (!rust::eq(this->sim.v2_cache.station_info[this->station].due_try_time.value(), this->time))
    {
        logs_cargo(
            "Error",
            "station {} try to process one but due time is {}, cur time {}",
            this->station,
            this->sim.v2_cache.station_info[this->station].due_try_time.value(),
            this->time
        );
        return;
    }
    this->sim.v2_cache.station_info[this->station].due_try_time.reset();

    std::ofstream number_package_in_station("number_package_in_station.csv", std::ios::app);
    std::ofstream package_trip("package_trip.csv", std::ios::app);

    logs("[{:.3f}] {}] station {} try to process one.", this->time, this->station, this->station);
    // 根据吞吐量判断 StartProcess 间隔
    // [处理 cd]
    if (!rust::time_ok(this->time, this->sim.stations.at(this->station).start_process_ok_time)) {
        // gg
        logs(
            "[{:.3f}] {}] station {} failed to process one package, because start-process is in cd",
            this->time,
            this->station,
            this->station
        );
        // when cd is ok, try again
        // [todo]
        try_due_try(
            this->sim.stations.at(this->station).start_process_ok_time,
            this->sim,
            this->station
        );
        return;
    }
    // [没东西]
    if (this->sim.stations.at(this->station).buffer.empty()) {
        logs(
            "[{:.3f}] {}] station {} failed to process one package, because there's no package.",
            this->time,
            this->station,
            this->station
        );
        return;
    }
    // [process success]
    struct ImTooLazy {
        DijRes dij;
        double time_to_ddl;
    };
    // auto estimated_time_to_ddl_if_process_now = [&](const string& pkg) {
    //     const double ddl = this->sim.packages[pkg].time_created
    //         + (this->sim.packages[pkg].category == PackageCategory::EXPRESS
    //                ? eval::V1_EXPRESS_DDL_HOURS
    //                : eval::V1_STANDARD_DDL_HOURS);
    //     auto res = dijkstra_tree(
    //         this->sim.stations,
    //         this->sim.routes,
    //         this->station,
    //         this->sim.packages[pkg].dst,
    //         this->time,
    //         this->sim.v2_cache.station_plans
    //     );
    //     return ImTooLazy { res, res.start_send_time_at_min_cost.at(this->sim.packages[pkg].dst) };
    // };
    auto tree = dijkstra_tree(
        this->sim.stations,
        this->sim.routes,
        this->station,
        this->time,
        this->sim.v2_cache.station_plans
    );
    auto estimated_time_to_ddl_if_process_now = [&](const string& pkg) {
        const double ddl = this->sim.packages[pkg].time_created
            + (this->sim.packages[pkg].category == PackageCategory::EXPRESS
                   ? eval::V1_EXPRESS_DDL_HOURS
                   : eval::V1_STANDARD_DDL_HOURS);
        return ddl - tree.start_send_time_at_min_cost.at(this->sim.packages[pkg].dst);
    };
    string vip_package = *this->sim.stations.at(this->station).buffer.begin();
    // auto vip_lazy = estimated_time_to_ddl_if_process_now(vip_package);
    // log::ecargo("V3", "---", vip_lazy.time_to_ddl);
    // log::ecargo("V3", "{}", vip_lazy.time_to_ddl);
    double vip_time_to_ddl = estimated_time_to_ddl_if_process_now(vip_package);
    for (const auto& package: this->sim.stations.at(this->station).buffer) {
        if (package == vip_package) {
            continue;
        }
        const double time_to_ddl = estimated_time_to_ddl_if_process_now(package);
        // log::ecargo("V3", "{} {}", package, time_to_ddl);
        if (time_to_ddl < vip_time_to_ddl) {
            vip_package = package;
            vip_time_to_ddl = time_to_ddl;
        }
    }
    // use dijkstra
    const vector<int> path = [&]() {
        vector<int> path;
        for (string at = this->sim.packages[vip_package].dst; at != this->station;) {
            auto [from, route] = tree.prev.at(at);
            // logs("from: {}, route: {}", from, route);
            path.push_back(route);
            at = from;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }();
    if (path.size() == 0) {
        // already at src
        logs(
            "[{:.3f}] {}] station {} is already at the src of {}, final process and SENT.",
            this->time,
            this->station,
            this->station,
            vip_package
        );
        assert(this->station == this->sim.packages[vip_package].dst);
        // this->sim.stations[this->station].buffer.erase(earliest);
        // this->sim.stations[this->station].processing_package = earliest;
        // 会修改 ok_time
        this->sim.stations.at(this->station)
            .take_package_from_buffer_to_processing(vip_package, this->time);
        for (const auto& [id, station]: this->sim.stations) {
            number_package_in_station << this->time << "," << id << ","
                                      << this->sim.stations.at(id).buffer.size() << "\n";
        }
        // 终点不 due
        // this->sim.v2_cache.station_plans.at().pop_due_pkg(earlist_package);

        // only station cost
        this->sim.add_transport_cost(this->sim.stations.at(this->station).cost);
        this->sim.schedule_event(new V3StartSend(
            // [todo]
            // 会预备一个 TryProcess，那么如何判断时间是否 ok?（注意精度问题）
            this->time + this->sim.stations.at(this->station).process_delay,
            this->sim,
            vip_package,
            this->station,
            -1
        ));
        try_due_try(
            this->sim.stations.at(this->station).start_process_ok_time,
            this->sim,
            this->station
        );
        package_trip << this->time << "," << vip_package << "," << this->station << ","
                     << this->station << "\n";
        return;
    }
    const string next_dst = this->sim.routes.at(this->station).at(path[0]).dst;
    logs(
        "[{:.3f}] {}] station {} process {} and send to station {}.",
        this->time,
        this->station,
        this->station,
        vip_package,
        next_dst
    );
    this->sim.stations.at(this->station)
        .take_package_from_buffer_to_processing(vip_package, this->time);
    for (const auto& [id, station]: this->sim.stations) {
        number_package_in_station << this->time << "," << id << ","
                                  << this->sim.stations.at(id).buffer.size() << "\n";
    }
    // choose path[0]
    // const string dst = this->sim.routes.at(this->station).at(path[0]).dst;
    this->sim.add_transport_cost(this->sim.routes.at(this->station).at(path[0]).cost);
    this->sim.add_transport_cost(this->sim.stations.at(this->station).cost);
    this->sim.schedule_event(new V3StartSend(
        this->time + this->sim.stations.at(this->station).process_delay,
        this->sim,
        vip_package,
        this->station,
        path[0]
    ));
    this->sim.v2_cache.station_plans.at(next_dst).add_due_pkg(
        this->time + this->sim.stations.at(this->station).process_delay
            + this->sim.routes.at(this->station).at(path[0]).time,
        vip_package
    );
    try_due_try(
        this->sim.stations.at(this->station).start_process_ok_time,
        this->sim,
        this->station
    );
    package_trip << this->time << "," << vip_package << "," << this->station << ","
                 << this->sim.routes.at(this->station).at(path[0]).dst << "\n";
}

void V3Arrival::process_event() {
    // std::ofstream file("output.txt", std::ios::app);
    // std::ofstream file("output.txt", std::ios::app);
    std::ofstream number_package_in_station("number_package_in_station.csv", std::ios::app);
    std::ofstream package_trip("package_trip.csv", std::ios::app);

    logs(
        "[{:.3f}] {}] Arrival pack {}: {}",
        this->time,
        this->station,
        this->package,
        this->station
    );
    if (!this->is_start) {
        this->sim.v2_cache.station_plans.at(this->station).pop_due_pkg(this->time, this->package);
    }
    this->sim.stations.at(this->station).buffer.insert(this->package);

    for (const auto& [id, station]: this->sim.stations) {
        number_package_in_station << this->time << "," << id << ","
                                  << this->sim.stations.at(id).buffer.size() << "\n";
    }
    package_trip << this->time << "," << this->package << "," << this->station << ","
                 << this->station << "\n";
    try_due_try(this->time, this->sim, this->station);
    // this->sim.schedule_event();
    // [test]
    // buffer size
    // logs("[{:.3f}] buffer size: {}", this->time, this->sim.stations[this->station].buffer.size());
}

void V3StartSend::process_event() {
    // turn into fmt
    // std::ofstream file("output.txt", std::ios::app);
    if (this->sim.packages[this->package].dst == this->src) {
        logs(
            "[{:.3f}] {}] StartSend pack {}: {} => {}, time {}",
            this->time,
            this->src,
            this->package,
            this->src,
            this->src,
            0
        );
        // package has arrived
        this->sim.finish_order(this->package, this->time);
        return;
    }
    logs(
        "[{:.3f}] {}] StartSend pack {}: {} => {}, time {}",
        this->time,
        this->src,
        this->package,
        this->src,
        this->sim.routes.at(this->src).at(this->route).dst,
        this->sim.routes.at(this->src).at(this->route).time
    );
    this->sim.schedule_event(new V3Arrival(
        // find src => dst route
        this->time + this->sim.routes.at(this->src).at(this->route).time,
        this->sim,
        this->package,
        this->sim.routes.at(this->src).at(this->route).dst,
        false // not start
    ));
    // try process one right now (but after this StartSend guranteed by event push)
    // this->sim.schedule_event(new V3TryProcessOne(this->time, this->sim, this->src));
}

} // namespace strategy::v3
