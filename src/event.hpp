#ifndef EVENT_HPP
#define EVENT_HPP

namespace sim {
struct Simulation;
}

namespace event {
using sim::Simulation;
struct Event {
public:
    const double time;

    Event(double t, Simulation& sim): time(t), sim(sim) {}
    virtual void process_event() = 0;
    virtual ~Event() = default;

protected:
    Simulation& sim;
};

struct EventComparator {
    bool operator()(Event* e1, Event* e2) {
        return e1->time > e2->time;
    }
};

} // namespace event

#endif
