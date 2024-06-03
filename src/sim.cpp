#include "sim.hpp"

#include "strategy/v1.hpp"

namespace sim {

using strategy::v1::V1Arrival;

void Simulation::run() {
    // chrono
    auto start_time = std::chrono::high_resolution_clock::now();
    while (!this->event_queue.empty()) {
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
    logs_cargo(
        "Run",
        "{}ms spent for simulation",
        std::chrono::duration<double, std::milli>(spent_run_time).count()
    );
}

void Simulation::add_order(string id, double time, PackageCategory ctg, string src, string dst) {
    // this->id_cnt += 1;
    // string id = std::to_string(this->id_cnt);
    this->packages[id] = Package { id, ctg, time, src, dst, false, 0.0 };
    if (this->strategy_version == StrategyVersion::V1
        || this->strategy_version == StrategyVersion::V1B)
    {
        this->schedule_event(new V1Arrival(time, *this, id, src));
    }
}

} // namespace sim