#include "strategy/v2.hpp"

#include "sim.hpp"

namespace strategy::v2 {

double StationPlan::next_arrival_time() {
    if (arrival_time_of_due_pkgs.empty()) {
        return std::numeric_limits<double>::max();
    }
    // return *arrival_time_of_due_pkgs.begin();
    return arrival_time_of_due_pkgs.top();
}

double StationPlan::expected_wait_time(double now) {
    int buffer_size = this->sim.stations[id].buffer.size();
    if (now >= this->next_arrival_time()) {
        buffer_size += this->arrival_time_of_due_pkgs.size();
    }
    const double buffer_clean_time = buffer_size / this->sim.stations[id].throughput;
    const double can_process_time = now - this->sim.stations[id].start_process_ok_time;
    const double wait_time = std::max(0.0, buffer_clean_time - can_process_time);
    return wait_time;
}
void StationPlan::add_due_pkg(double t) {
    this->arrival_time_of_due_pkgs.push(t);
}
void StationPlan::pop_due_pkg(double t) {
    assert(!this->arrival_time_of_due_pkgs.empty());
    assert(rust::eq(this->arrival_time_of_due_pkgs.top(), t));
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
        this->sim.schedule_event(new V2TryProcessOne(
            this->sim.stations.at(this->station).start_process_ok_time,
            this->sim,
            this->station
        ));
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
    string earlist_package = *this->sim.stations.at(this->station).buffer.begin();
    for (const auto& package: this->sim.stations.at(this->station).buffer) {
        if (this->sim.packages[package].time_created
            < this->sim.packages[earlist_package].time_created)
        {
            earlist_package = package;
        }
    }
    // use dijkstra
    auto path = [&]() {
        if (this->sim.strategy_version == StrategyVersion::V1B) {
            return dijkstra_enhanced(
                this->sim.stations,
                this->sim.routes,
                this->station,
                this->sim.packages[earlist_package].dst
            );
        }
        return dijkstra(
            this->sim.stations,
            this->sim.routes,
            this->station,
            this->sim.packages[earlist_package].dst
        );
    }();
    if (path.size() == 0) {
        // already at src
        logs(
            "[{:.3f}] {}] station {} is already at the src of {}, final process and SENT.",
            this->time,
            this->station,
            this->station,
            earlist_package
        );
        assert(this->station == this->sim.packages[earlist_package].dst);
        // this->sim.stations[this->station].buffer.erase(earliest);
        // this->sim.stations[this->station].processing_package = earliest;
        // 会修改 ok_time
        this->sim.stations.at(this->station)
            .take_package_from_buffer_to_processing(earlist_package, this->time);
        for (const auto& [id, station]: this->sim.stations) {
            number_package_in_station << this->time << "," << id << ","
                                      << this->sim.stations.at(id).buffer.size() << "\n";
        }
        // only station cost
        this->sim.add_transport_cost(this->sim.stations.at(this->station).cost);
        this->sim.schedule_event(new V2StartSend(
            // [todo]
            // 会预备一个 TryProcess，那么如何判断时间是否 ok?（注意精度问题）
            this->time + this->sim.stations.at(this->station).process_delay,
            this->sim,
            earlist_package,
            this->station,
            -1
        ));
        this->sim.schedule_event(new V2TryProcessOne(
            this->sim.stations.at(this->station).start_process_ok_time,
            this->sim,
            this->station
        ));
        package_trip << this->time << "," << earlist_package << "," << this->station << ","
                     << this->station << "\n";
        return;
    }
    logs(
        "[{:.3f}] {}] station {} process {} and send to station {}.",
        this->time,
        this->station,
        this->station,
        earlist_package,
        this->sim.routes.at(this->station).at(path[0]).dst
    );
    this->sim.stations.at(this->station)
        .take_package_from_buffer_to_processing(earlist_package, this->time);
    for (const auto& [id, station]: this->sim.stations) {
        number_package_in_station << this->time << "," << id << ","
                                  << this->sim.stations.at(id).buffer.size() << "\n";
    }
    // choose path[0]
    this->sim.add_transport_cost(this->sim.routes.at(this->station).at(path[0]).cost);
    this->sim.add_transport_cost(this->sim.stations.at(this->station).cost);
    this->sim.schedule_event(new V2StartSend(
        this->time + this->sim.stations.at(this->station).process_delay,
        this->sim,
        earlist_package,
        this->station,
        path[0]
    ));
    package_trip << this->time << "," << earlist_package << "," << this->station << ","
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
    this->sim.stations.at(this->station).buffer.insert(this->package);

    for (const auto& [id, station]: this->sim.stations) {
        number_package_in_station << this->time << "," << id << ","
                                  << this->sim.stations.at(id).buffer.size() << "\n";
    }
    package_trip << this->time << "," << this->package << "," << this->station << ","
                 << this->station << "\n";
    this->sim.schedule_event(new V2TryProcessOne(this->time, this->sim, this->station));
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
        this->sim.routes.at(this->src).at(this->route).dst
    ));
    // try process one right now (but after this StartSend guranteed by event push)
    // this->sim.schedule_event(new V2TryProcessOne(this->time, this->sim, this->src));
}

} // namespace strategy::v2
