#include "sim.hpp"

#include "strategy/v1.hpp"
#include "strategy/v2.hpp"

namespace sim {

using strategy::v1::V1Arrival;

void Simulation::run() {
    std::ofstream num_p("number_package_in_station.csv", std::ios::trunc);
    num_p.clear();
    std::ofstream package_trip("package_trip.csv", std::ios::trunc);
    package_trip.clear();

    // chrono
    auto start_time = std::chrono::high_resolution_clock::now();
    while (!this->event_queue.empty()) {
        this->event_cnt += 1;
        Event* event = this->event_queue.top();
        this->event_queue.pop();
        this->current_time = event->time;
        event->process_event();

        // cout << "arrived: " << this->arrived << ", "; // "arrived: 1\n"
        // cout << "time cost: " << this->total_time << "\n";
        // in fmt
        logs("arrived: {}, money cost: {}", this->arrived, this->transport_cost);
        delete event;
    }
    auto spent_run_time = std::chrono::high_resolution_clock::now() - start_time;
    log::ecargo(
        "Run",
        "{}ms spent for simulation",
        std::chrono::duration<double, std::milli>(spent_run_time).count()
    );
}

void Simulation::add_order(string id, double time, PackageCategory ctg, string src, string dst) {
    // this->id_cnt += 1;
    // string id = std::to_string(this->id_cnt);
    this->packages[id] = Package { id, ctg, time, src, dst, false, -10086 };
    if (this->strategy_version == StrategyVersion::V1
        || this->strategy_version == StrategyVersion::V1B)
    {
        this->schedule_event(new V1Arrival(time, *this, id, src));
    } else if (this->strategy_version == StrategyVersion::V2
               || this->strategy_version == StrategyVersion::V2B)
    {
        this->schedule_event(new strategy::v2::V2Arrival(time, *this, id, src, true));
    }
}

} // namespace sim
