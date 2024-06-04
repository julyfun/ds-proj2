#include "strategy/v2.hpp"

#include "base.hpp"
#include "log.hpp"
#include "sim.hpp"
#include "strategy.hpp"

namespace strategy::v2 {

using std::make_pair;

void try_due_try(double t, Simulation& sim, string station) {
    // check cached due time
    if (!sim.v2_cache.station_info.at(station).due_try_time.has_value()) {
        sim.v2_cache.station_info.at(station).due_try_time = t;
        sim.schedule_event(new V2TryProcessOne(t, sim, station));
    }
    if (t < sim.v2_cache.station_info.at(station).due_try_time) {
        sim.v2_cache.station_info.at(station).due_try_time = t;
        sim.schedule_event(new V2TryProcessOne(t, sim, station));
        return;
    }
}

vector<int> fake_dijkstra(
    const map<string, Station>& stations,
    const map<string, map<int, Route>>& routes,
    string src,
    string dst,
    double start_process_time,
    const map<string, StationPlan>& station_plans,
    double money_coefficient = 1.0,
    double time_coefficient = 1.667
) {
    logs_cargo("Info", "fake_dijkstra called");
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

    vector<int> path;
    // for (string at = dst; at != ""; at = prev[at]) {
    //     path.push_back(at);
    // }
    for (string at = dst; at != src;) {
        auto [from, route] = prev[at];
        // logs("from: {}, route: {}", from, route);
        path.push_back(route);
        at = from;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

double StationPlan::next_arrival_time() const {
    if (arrival_time_of_due_pkgs.empty()) {
        return std::numeric_limits<double>::max();
    }
    // return *arrival_time_of_due_pkgs.begin();
    return arrival_time_of_due_pkgs.top().first;
}

double StationPlan::estimated_wait_time(double now, double arrive_time) const {
    // int buffer_size = this->sim.stations[id].buffer.size();
    // if (arrive_time >= this->next_arrival_time()) {
    //     buffer_size += this->arrival_time_of_due_pkgs.size();
    // }
    // logs_cargo("Estimate", "{} {} {}", arrive_time, this->next_arrival_time(), buffer_size);
    // const double buffer_clean_time = buffer_size / this->sim.stations[id].throughput;
    // const double can_process_time = arrive_time - this->sim.stations[id].start_process_ok_time;
    // const double wait_time = std::max(0.0, buffer_clean_time - can_process_time);
    // return wait_time;
    const Station& station = this->sim.stations.at(id);
    const double finish_cur_buf_time = [&]() {
        if (station.buffer.size() == 0) {
            return now;
        }
        return std::max(now, station.start_process_ok_time)
            + station.buffer.size() / station.throughput;
    }();
    const double next_due_arrive_time = this->next_arrival_time();
    // （在最简单的策略下）我先到，我先处理
    // 若没有 due 则就会执行这一步
    if (arrive_time < next_due_arrive_time) {
        return std::max(0.0, finish_cur_buf_time - arrive_time);
    }
    // 假定另外一个包先到就需要处理所有 due 包（这是为了省算力）
    const double finish_all_other_dues_time = std::max(finish_cur_buf_time, next_due_arrive_time)
        + this->arrival_time_of_due_pkgs.size() / station.throughput;
    return std::max(0.0, finish_all_other_dues_time - arrive_time);
}
void StationPlan::add_due_pkg(double t, const string& id) {
    this->arrival_time_of_due_pkgs.push(make_pair(t, id));
}
void StationPlan::pop_due_pkg(double t, const string& id) {
    assert(!this->arrival_time_of_due_pkgs.empty());
    assert(rust::eq(this->arrival_time_of_due_pkgs.top().first, t));
    assert(this->arrival_time_of_due_pkgs.top().second == id);
    // if (this->arrival_time_of_due_pkgs.empty() || this->arrival_time_of_due_pkgs.top().second != id)
    // {
    //     return;
    // }
    this->arrival_time_of_due_pkgs.pop();
}

V2Cache::V2Cache(Simulation& sim) {
    for (const auto& [id, station]: sim.stations) {
        station_plans.emplace(id, StationPlan(id, sim));
    }
}

void V2TryProcessOne::process_event() {
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
    string vip_package = *this->sim.stations.at(this->station).buffer.begin();
    if (this->sim.strategy_version == StrategyVersion::V2) {
        for (const auto& package: this->sim.stations.at(this->station).buffer) {
            if (this->sim.packages[package].time_created
                < this->sim.packages[vip_package].time_created)
            {
                vip_package = package;
            }
        }
    } else { // V2B
        for (const auto& package: this->sim.stations.at(this->station).buffer) {
            // if (this->sim.packages[package].time_created
            //     < this->sim.packages[earlist_package].time_created)
            // {
            //     earlist_package = package;
            // }
            // EXPRESS first
            const bool vip_is_express =
                this->sim.packages[vip_package].category == PackageCategory::EXPRESS;
            const bool package_is_express =
                this->sim.packages[package].category == PackageCategory::EXPRESS;
            if (vip_is_express != package_is_express) {
                if (package_is_express) {
                    vip_package = package;
                }
                continue;
            }
            if (this->sim.packages[package].time_created
                < this->sim.packages[vip_package].time_created)
            {
                vip_package = package;
            }
        }
    }

    // use dijkstra
    auto path = [&]() {
        return fake_dijkstra(
            this->sim.stations,
            this->sim.routes,
            this->station,
            this->sim.packages[vip_package].dst,
            this->time,
            this->sim.v2_cache.station_plans
        );
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
        this->sim.schedule_event(new V2StartSend(
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
    logs(
        "[{:.3f}] {}] station {} process {} and send to station {}.",
        this->time,
        this->station,
        this->station,
        vip_package,
        this->sim.routes.at(this->station).at(path[0]).dst
    );
    this->sim.stations.at(this->station)
        .take_package_from_buffer_to_processing(vip_package, this->time);
    for (const auto& [id, station]: this->sim.stations) {
        number_package_in_station << this->time << "," << id << ","
                                  << this->sim.stations.at(id).buffer.size() << "\n";
    }
    // choose path[0]
    const string dst = this->sim.routes.at(this->station).at(path[0]).dst;
    this->sim.add_transport_cost(this->sim.routes.at(this->station).at(path[0]).cost);
    this->sim.add_transport_cost(this->sim.stations.at(this->station).cost);
    this->sim.schedule_event(new V2StartSend(
        this->time + this->sim.stations.at(this->station).process_delay,
        this->sim,
        vip_package,
        this->station,
        path[0]
    ));
    this->sim.v2_cache.station_plans.at(dst).add_due_pkg(
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

void V2Arrival::process_event() {
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

void V2StartSend::process_event() {
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
    this->sim.schedule_event(new V2Arrival(
        // find src => dst route
        this->time + this->sim.routes.at(this->src).at(this->route).time,
        this->sim,
        this->package,
        this->sim.routes.at(this->src).at(this->route).dst,
        false // not start
    ));
    // try process one right now (but after this StartSend guranteed by event push)
    // this->sim.schedule_event(new V2TryProcessOne(this->time, this->sim, this->src));
}

} // namespace strategy::v2
