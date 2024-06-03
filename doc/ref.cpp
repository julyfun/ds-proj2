#include <iostream>
#include <queue>
#include <vector>

// Return a random integer in the range 0 to n-1.
int irand(int n) {
    return rand() % n;
}

class event {
public:
    // Construct sets time of event.
    event(unsigned int t): time(t) {}

    // Execute event by invoking this method.
    virtual void processEvent() = 0;

    const unsigned int time;
};

struct eventComparator {
    bool operator()(const event* left, const event* right) const {
        return left->time > right->time;
    }
};

class simulation {
public:
    simulation(): time(0), eventQueue() {}
    void run();
    void scheduleEvent(event* newEvent) {
        eventQueue.push(newEvent);
    }
    unsigned int time;

protected:
    std::priority_queue<event*, std::vector<event*, std::allocator<event*>>, eventComparator>
        eventQueue;
};

void simulation::run() {
    while (!eventQueue.empty()) {
        event* nextEvent = eventQueue.top();
        eventQueue.pop();
        time = nextEvent->time;
        nextEvent->processEvent();
        delete nextEvent;
    }
}

class storeSimulation: public simulation {
public:
    storeSimulation(): simulation(), freeChairs(35), profit(0.0) {}
    bool canSeat(unsigned int numberOfPeople);
    void order(unsigned int numberOfScoops);
    void leave(unsigned int numberOfPeople);
    // Data fields.
    unsigned int freeChairs;
    double profit;
} theSimulation;

bool storeSimulation::canSeat(unsigned int numberOfPeople) {
    std::cout << "Time: " << time;
    std::cout << " group of " << numberOfPeople << " customers arrives";

    if (numberOfPeople < freeChairs) {
        std::cout << " is seated\n";
        freeChairs -= numberOfPeople;
        return true;
    } else {
        std::cout << " no room, they leave\n";
        return false;
    }
}

void storeSimulation::order(unsigned int numberOfScoops) {
    std::cout << "Time: " << time << " serviced order for " << numberOfScoops << '\n';
    profit += 0.35 * numberOfScoops;
}

void storeSimulation::leave(unsigned int numberOfPeople) {
    std::cout << "Time: " << time << " group of size " << numberOfPeople << " leaves\n";
    freeChairs += numberOfPeople;
}

class leaveEvent: public event {
public:
    leaveEvent(unsigned int t, unsigned int groupSize): event(t), size(groupSize) {}
    virtual void processEvent();

private:
    unsigned int size;
};

void leaveEvent::processEvent() {
    theSimulation.leave(size);
}

class orderEvent: public event {
public:
    orderEvent(unsigned int t, unsigned int groupSize): event(t), size(groupSize) {}
    virtual void processEvent();

private:
    unsigned int size;
};

void orderEvent::processEvent() {
    // Each person orders some number of scoops.
    for (unsigned int i = 0; i < size; i++)
        theSimulation.order(1 + irand(4));

    // Then we schedule the leave event.
    theSimulation.scheduleEvent(new leaveEvent(time + 1 + irand(10), size));
}

class arriveEvent: public event {
public:
    arriveEvent(unsigned int t, unsigned int groupSize): event(t), size(groupSize) {}
    virtual void processEvent();

private:
    unsigned int size;
};

void arriveEvent::processEvent() {
    if (theSimulation.canSeat(size))
        theSimulation.scheduleEvent(new orderEvent(time + 1 + irand(4), size));
}

int main() {
    std::cout << "Ice Cream Store simulation from Chapter 9\n";

    // Load queue with some number of initial events.
    for (unsigned t = 0; t < 20; t += irand(6)) {
        std::cout << "pumping queue with event " << t << '\n';
        theSimulation.scheduleEvent(new arriveEvent(t, 1 + irand(4)));
    }

    // Run the simulation.
    theSimulation.run();

    std::cout << "Total profits " << theSimulation.profit
              << "\nEnd of ice cream store simulation\n";

    return 0;
}
