#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

using std::cout;
using std::make_pair;
using std::map;
using std::pair;
using std::priority_queue;
using std::set;
using std::string;
using std::vector;

int rand(int n) {
    return rand() % n;
}

class Simulation;

class Event {
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
    string src;
    string dst;
    double time;
    double cost;
};

struct Station {
    string id;
    double time_on_belt;
    double time_process;
    set<string> unprocessed_packages;
};

enum class PackageCategory {
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

class Simulation {
private:
    std::priority_queue<Event*, std::vector<Event*, std::allocator<Event*>>, EventComparator>
        event_queue;
    int arrived = 0;
    double total_time = 0;

    int id = 0;

public:
    map<string, Station> stations;
    map<string, vector<Route>> routes;
    map<string, Package> packages;

public:
    double time;

    void run();
    void schedule_event(Event* event) {
        this->event_queue.push(event);
    }
    void add_order(double time, PackageCategory ctg, string src, string dst) {
        this->id += 1;
        string id = std::to_string(this->id);
        this->packages[id] = Package { id, ctg, time, src, dst };
    }
    // add station
    void add_station(string id, double time_on_belt, double time_process) {
        Station station;
        station.id = id;
        station.time_on_belt = time_on_belt;
        station.time_process = time_process;
        this->stations[id] = station;
    }
    // add route
    void add_route(string src, string dst, double time, double cost) {
        auto route = Route { src, dst, time, cost };
        this->routes[src].push_back(route);
    }
    // arrive package
    void arrive(string package, double time) {
        this->total_time += time - this->packages[package].time_created;
        this->arrived += 1;
    }
};

void Simulation::run() {
    while (!this->event_queue.empty()) {
        Event* event = this->event_queue.top();
        this->event_queue.pop();
        this->time = event->time;
        event->process_event();
        cout << "arrived: " << this->arrived << ", "; // "arrived: 1\n"
        cout << "time cost: " << this->total_time << "\n";
        delete event;
    }
}

class PackageArrival: public Event {
public:
    PackageArrival(double t, Simulation& sim, string package, string dst):
        Event(t, sim),
        package(package),
        dst(dst) {}

    void process_event() override;

private:
    string package;
    string dst;
};

class PackageStartBelt: public Event {
public:
    PackageStartBelt(double t, Simulation& sim, string package, string src, string dst):
        Event(t, sim),
        package(package),
        src(src),
        dst(dst) {}

    void process_event() override;

private:
    string package;
    string src;
    string dst;
};

class PackageStartProcessing: public Event {
public:
    PackageStartProcessing(double t, Simulation& sim, string package, string src, string dst):
        Event(t, sim),
        package(package),
        src(src),
        dst(dst) {}

    void process_event() override;

private:
    string package;
    string src;
    string dst;
};

// [可能是送原地，即完成配送]
class PackageStartSend: public Event {
public:
    PackageStartSend(double t, Simulation& sim, string package, string src, string dst):
        Event(t, sim),
        package(package),
        src(src),
        dst(dst) {}

    void process_event() override {
        cout << "[" << this->time << "] PackageStartSend pack" << this->package << ": " << this->src
             << " => " << this->dst << "\n";
        if (this->sim.packages[this->package].dst == this->src) {
            // package has arrived
            this->sim.arrive(this->package, this->time);
            return;
        }
        this->sim.schedule_event(new PackageArrival(
            this->time + this->sim.routes[this->src][0].time,
            this->sim,
            this->package,
            this->dst
        ));
    }

private:
    string package;
    string src;
    string dst;
};

void PackageArrival::process_event() {
    cout << "[" << this->time << "] PackageArrival pack" << this->package << ": " << this->dst
         << "\n";
    this->sim.schedule_event(
        new PackageStartBelt(this->time, this->sim, this->package, this->dst, this->dst)
    );
}

void PackageStartBelt::process_event() {
    cout << "[" << this->time << "] PackageStartBelt pack" << this->package << ": " << this->src
         << " => " << this->dst << "\n";
    this->sim.schedule_event(new PackageStartProcessing(
        this->time + this->sim.stations[this->src].time_on_belt,
        this->sim,
        this->package,
        this->src,
        this->dst
    ));
}

void PackageStartProcessing::process_event() {
    cout << "[" << this->time << "] PackageStartProcessing pack" << this->package << ": "
         << this->src << " => " << this->dst << "\n";
    this->sim.schedule_event(new PackageStartSend(
        this->time + this->sim.stations[this->src].time_process,
        this->sim,
        this->package,
        this->src,
        this->dst
    ));
}

int main() {
    cout << "Begin\n";
    Simulation sim;
    sim.add_station("a", 1.0 / 6, 2);
    sim.add_station("b", 1.0 / 5, 2);
    sim.add_route("a", "b", 100, 1200);
    sim.add_order(100, PackageCategory::STANDARD, "a", "b");
    sim.schedule_event(new PackageStartBelt(102, sim, "1", "a", "b"));
    sim.run();
    cout << "End\n";
    return 0;
}
