#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "fmt/core.h"
#include "log.hpp"
#include "rust.hpp"

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

// [comptime]

// [const]

int rand(int n) {
    return rand() % n;
}

struct Simulation;

struct Event {
public:
    const double time;

    Event(double t, Simulation& sim): time(t), sim(sim) {}
    virtual void process_event() = 0;

protected:
    Simulation& sim;
};

struct EventComparator {
    bool operator()(Event* e1, Event* e2) {
        return e1->time > e2->time;
    }
};

struct Route {
    int id;
    string src;
    string dst;
    double time;
    double cost;
};

enum struct TripType {
    ARRIVED,
    PROCESSING,
    SENT,
};

struct TripInfo {
    double time;
    string package;
    string location;
    TripType type;
};

struct DataBase {
    map<string, vector<TripInfo>> package_trips;
};

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
// }

// minimal
struct Station {
public:
    string id;
    double throughput;
    double process_time;

    set<string> buffer;
    // 下一个可以开始处理的时间
    double start_process_ok_time = std::numeric_limits<double>::min();
    // optional<string> processing_package;

public:
    void take_package_from_buffer_to_processing(string package, double time) {
        this->buffer.erase(package);
        this->start_process_ok_time = time + 1.0 / this->throughput;
        // this->processing_package = package;
    }
};

enum struct PackageCategory {
    STANDARD,
    EXPRESS,
};

struct Package {
    string id;
    PackageCategory category;
    double time_created;
    string src;
    string dst;
};

enum struct EvaluateStrategy {
    V0,
    V1,
    V2,
};

struct PackageDynamicInfo {
    string id;
    bool finished;
    double time_finished;
};

struct EvalFunc {
    virtual double operator()(
        double transport_cost,
        const map<string, Package>& pkg,
        const map<string, PackageDynamicInfo>& dyn
    ) = 0;
};

struct EvalFuncV0: public EvalFunc {
    double operator()(
        double transport_cost,
        const map<string, Package>& pkgs,
        const map<string, PackageDynamicInfo>& dyn
    ) override {
        double tot_cost = transport_cost;
        for (const auto& [id, pkg]: pkgs) {
            if (!dyn.at(id).finished) {
                tot_cost += 1e6;
                logs("package {} not finished", id);
                continue;
            }
            double time_cost = dyn.at(id).time_finished - pkg.time_created;
            double cost = time_cost * (pkg.category == PackageCategory::EXPRESS ? 10 : 5);
            logs("package {} finished at {}, cost {}", id, dyn.at(id).time_finished, cost);
            tot_cost += cost;
        }
        return tot_cost;
    }
};

struct Simulation {
private:
    double current_time; // current time
    std::priority_queue<Event*, std::vector<Event*, std::allocator<Event*>>, EventComparator>
        event_queue;
    int arrived = 0;
    int route_cnt = 0;
    double transport_cost = 0;

    // EvaluateStrategy evaluate_strategy = EvaluateStrategy::V0;
    map<string, PackageDynamicInfo> package_dynamic_info;

public:
    DataBase db;
    map<string, Station> stations;
    map<string, map<int, Route>> routes;
    map<string, Package> packages;

    // map<string, map<string,

public:
    void run();

    void schedule_event(Event* event) {
        this->event_queue.push(event);
    }

    void
    add_order(string id, double time, PackageCategory ctg, string src, string dst); // add station

    void add_transport_cost(double cost) {
        this->transport_cost += cost;
    }

    void add_station(string id, double throughput, double process_time) {
        this->stations[id] = Station { id, throughput, process_time };
    }
    // add route
    void add_route(string src, string dst, double time, double cost) {
        // check src and dst exist
        assert(this->stations.find(src) != this->stations.end());
        assert(this->stations.find(dst) != this->stations.end());
        this->route_cnt += 1;
        auto route = Route { this->route_cnt, src, dst, time, cost };
        this->routes[src].emplace(this->route_cnt, route); // add key value
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
        this->package_dynamic_info[package].finished = true;
        this->package_dynamic_info[package].time_finished = time;
    }
    double eval(EvalFunc&& eval_func) {
        return eval_func(this->transport_cost, this->packages, this->package_dynamic_info);
    }
};

void Simulation::run() {
    std::ofstream number_package_in_station("number_package_in_station.csv");
    number_package_in_station.clear();
    std::ofstream package_trip("package_trip.csv");
    package_trip.clear();

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
}

struct Arrival: public Event {
public:
    Arrival(double t, Simulation& sim, string package, string dst):
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

// 使用堆优化 dijkstra 求解时间最短路，返回最短路整条路径 id vector
// routes[x] is all routes of x
// routes[x][rid] is route of x
vector<int> dijkstra(
    const map<string, Station>& stations,
    const map<string, map<int, Route>>& routes,
    string src,
    string dst
) {
    map<string, double> dist;
    map<string, pair<string, int>> prev; // nodes' prev station and route
    for (const auto& [id, station]: stations) {
        dist[id] = std::numeric_limits<double>::max();
    }
    dist[src] = 0;
    // priority_queue<pair<double, string>> q;
    priority_queue<
        pair<double, string>,
        vector<pair<double, string>>,
        greater<pair<double, string>>>
        q;

    q.push(make_pair(0, src));
    while (!q.empty()) {
        auto [d, u] = q.top();
        q.pop();
        if (d > dist[u]) {
            continue;
        }
        // if not found, edges is empty
        auto edges = routes.find(u) == routes.end() ? map<int, Route> {} : routes.at(u);
        for (const auto& route: edges) {
            // cout << "  route: " << route.src << " => " << route.dst << "\n";
            string v = route.second.dst;
            double w = route.second.time;
            // cout << "  dist v: " << dist[v] << ", dist u: " << dist[u] << ", w: " << w << "\n";
            if (dist[u] + w < dist[v]) {
                // cout << "    update dist: " << dist[u] + w << "\n";
                dist[v] = dist[u] + w;
                prev[v] = { u, route.first };
                // cout << "    prev: " << prev[v] << "\n";
                q.push(make_pair(dist[v], v));
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

TEST_CASE("dijkstra") {
    auto stations = map<string, Station> {
        { "a", Station { "a", 1.0 / 6, 2 } }, { "b", Station { "b", 1.0 / 5, 2 } },
        { "c", Station { "c", 1.0 / 4, 2 } }, { "d", Station { "d", 1.0 / 3, 2 } },
        { "e", Station { "e", 1.0 / 2, 2 } },
    };
    map<string, map<int, Route>> routes;
    routes["a"] = {
        { 1, Route { 1, "a", "b", 1, 1 } },
        { 7, Route { 7, "a", "b", 0.9, 1 } },
        { 8, Route { 8, "a", "b", 2, 1 } },
        { 2, Route { 2, "a", "c", 2, 2 } },
    };
    routes["b"] = {
        { 3, Route { 3, "b", "c", 3, 3 } },
        { 4, Route { 4, "b", "d", 4, 4 } },
    };
    routes["c"] = {
        { 5, Route { 5, "c", "d", 5, 5 } },
    };
    routes["d"] = {
        { 6, Route { 6, "d", "e", 6, 6 } },
    };
    auto path = dijkstra(stations, routes, "a", "e");
    // for (const auto& station: path) {
    //     cout << station << " ";
    // }
    // cout << '\n';
    auto ans = vector<int> { 7, 4, 6 };
    for (int i = 0; i < path.size(); i++) {
        CHECK(path[i] == ans[i]);
    }
}

// 检查 buffer 和从 buffer 中拿出内容 buffer 必须在同一个 event
struct V1TryProcessOne: public Event {
private:
    string station;

public:
    V1TryProcessOne(double t, Simulation& sim, string station): Event(t, sim), station(station) {}

    void process_event() override {
        // std::ofstream file("output.txt", std::ios::app);
        // std::ofstream file("output.txt", std::ios::app);
        std::ofstream number_package_in_station("number_package_in_station.csv", std::ios::app);
        std::ofstream package_trip("package_trip.csv", std::ios::app);

        logs(
            "[{:.3f}] {}] station {} try to process one.",
            this->time,
            this->station,
            this->station
        );
        // 根据吞吐量判断 StartProcess 间隔
        if (!rust::time_ok(this->time, this->sim.stations[this->station].start_process_ok_time)) {
            // gg
            logs(
                "[{:.3f}] {}] station {} failed to process one package, because start-process is in cd",
                this->time,
                this->station,
                this->station
            );
            // when cd is ok, try again
            this->sim.schedule_event(new V1TryProcessOne(
                this->sim.stations[this->station].start_process_ok_time,
                this->sim,
                this->station
            ));

            number_package_in_station << this->time << "," << this->station << ","
                                      << this->sim.stations[this->station].buffer.size() << "\n";
            return;
        }
        if (this->sim.stations[this->station].buffer.empty()) {
            logs(
                "[{:.3f}] {}] station {} failed to process one package, because there's no package.",
                this->time,
                this->station,
                this->station
            );
            return;
        }
        // [process success]
        string earlist_package = *this->sim.stations[this->station].buffer.begin();
        for (const auto& package: this->sim.stations[this->station].buffer) {
            if (this->sim.packages[package].time_created
                < this->sim.packages[earlist_package].time_created)
            {
                earlist_package = package;
            }
        }
        // use dijkstra
        auto path = dijkstra(
            this->sim.stations,
            this->sim.routes,
            this->station,
            this->sim.packages[earlist_package].dst
        );
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
            this->sim.stations[this->station].take_package_from_buffer_to_processing(
                earlist_package,
                this->time
            );
            for (const auto& [id, station]: this->sim.stations) {
                number_package_in_station << this->time << "," << id << "," << station.buffer.size()
                                          << "\n";
            }
            this->sim.add_transport_cost(0); // no cost
            this->sim.schedule_event(new V1StartSend(
                // [todo]
                // 会预备一个 TryProcess，那么如何判断时间是否 ok?（注意精度问题）
                this->time + this->sim.stations[this->station].process_time,
                this->sim,
                earlist_package,
                this->station,
                0
            ));
            this->sim.schedule_event(new V1TryProcessOne(
                this->sim.stations[this->station].start_process_ok_time,
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
            this->sim.routes[this->station][path[0]].dst
        );
        this->sim.stations[this->station].take_package_from_buffer_to_processing(
            earlist_package,
            this->time
        );
        number_package_in_station << this->time << "," << this->station << ","
                                  << this->sim.stations[this->station].buffer.size() << "\n";
        // choose path[0]
        this->sim.add_transport_cost(this->sim.routes[this->station][path[0]].cost);
        this->sim.schedule_event(new V1StartSend(
            this->time + this->sim.stations[this->station].process_time,
            this->sim,
            earlist_package,
            this->station,
            path[0]
        ));
        package_trip << this->time << "," << earlist_package << "," << this->station << ","
                     << path[0] << "\n";
    }
};

void Arrival::process_event() {
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
    this->sim.stations[this->station].buffer.insert(this->package);

    for (const auto& [id, station]: this->sim.stations) {
        number_package_in_station << this->time << "," << id << "," << station.buffer.size()
                                  << "\n";
    }
    package_trip << this->time << "," << this->package << "," << this->station << ","
                 << this->station << "\n";
    this->sim.schedule_event(new V1TryProcessOne(this->time, this->sim, this->station));
    // this->sim.schedule_event();
    // [test]
    // buffer size
    // logs("[{:.3f}] buffer size: {}", this->time, this->sim.stations[this->station].buffer.size());
}

void V0StartProcess::process_event() {
    std::ofstream package_trip("package_trip.csv", std::ios::app);
    logs(
        "[{:.3f}] {} StartProcess pack {}: {} => {}",
        this->time,
        this->src,
        this->src,
        this->package,
        this->src,
        this->sim.routes[this->src][this->route].dst
    );
    this->sim.schedule_event(new V1StartSend(
        this->time + this->sim.stations[this->src].process_time,
        this->sim,
        this->package,
        this->src,
        this->route
    ));
    package_trip << this->time << "," << this->package << "," << this->src << ","
                 << this->sim.routes[this->src][this->route].dst << "\n";
}

void V1StartSend::process_event() {
    // turn into fmt
    // std::ofstream file("output.txt", std::ios::app);
    logs(
        "[{:.3f}] {}] StartSend pack {}: {} => {}, time",
        this->time,
        this->src,
        this->package,
        this->src,
        this->sim.routes[this->src][this->route].dst,
        this->sim.routes[this->src][this->route].time
    );
    if (this->sim.packages[this->package].dst == this->src) {
        // package has arrived
        this->sim.finish_order(this->package, this->time);
        return;
    }
    this->sim.schedule_event(new Arrival(
        // find src => dst route
        this->time + this->sim.routes[this->src][this->route].time,
        this->sim,
        this->package,
        this->sim.routes[this->src][this->route].dst
    ));
    // try process one right now (but after this StartSend guranteed by event push)
    // this->sim.schedule_event(new V1TryProcessOne(this->time, this->sim, this->src));
}

void Simulation::add_order(string id, double time, PackageCategory ctg, string src, string dst) {
    // this->id_cnt += 1;
    // string id = std::to_string(this->id_cnt);
    this->packages[id] = Package { id, ctg, time, src, dst };
    this->package_dynamic_info[id] = PackageDynamicInfo { id, false, 0.0 };
    this->schedule_event(new Arrival(time, *this, id, src));
}

TEST_CASE("simple") {
    Simulation sim;
    sim.add_station("A", 5, 2);
    sim.add_station("B", 20, 2);
    sim.add_route("A", "B", 100, 50);
    sim.add_route("A", "B", 50, 10);
    sim.add_route("A", "B", 30, 66);
    sim.add_route("A", "B", 200, 10);
    sim.add_order("p1", 100, PackageCategory::STANDARD, "A", "B");
    sim.add_order("p2", 100, PackageCategory::EXPRESS, "A", "B");
    sim.run();
    logs("cost: {}", sim.eval(EvalFuncV0()));
}

TEST_CASE("buffer") {
    Simulation sim;
    sim.add_station("a", 10, 4.5);
    sim.add_station("b", 20, 2);
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

    std::ifstream file("../data.txt", std::ios::in);
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
                // cout << "id: " << id << " throughput: " << throughput
                //  << " time process:" << time_process << std::endl;
                sim.add_station(id, throughput, time_process);

            } else if (is_routes_section) {
                std::stringstream ss(line);
                string src;
                string dst;
                double time_cost, money_cost;
                char c;

                ss >> src >> c >> dst >> c >> time_cost >> c >> money_cost;

                // cout << "src: " << src << " dst: " << dst << " time cost: " << time_cost
                //      << " money_cost: " << money_cost << std::endl;
                sim.add_route(src, dst, time_cost, money_cost);
            } else if (is_orders_section) {
                std::stringstream ss(line);
                double time;
                int ctg;
                string src;
                string dst;
                string id;
                char c;

                ss >> id >> c >> time >> c >> ctg >> c >> src >> c >> dst;

                // cout << "time: " << time << " ctg: " << ctg << " src: " << src << " dst: " << dst
                //      << std::endl;
                if (ctg == 0)
                    sim.add_order(id, time, PackageCategory::STANDARD, src, dst);
                else
                    sim.add_order(id, time, PackageCategory::EXPRESS, src, dst);
            }
        }
    } else {
        std::cout << "File not found" << std::endl;
    }

    // sim.schedule_event(new TryProcessOneV1(102, sim, "a"));
    sim.run();
    cout << "End\n";
}
