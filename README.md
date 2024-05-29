事件驱动编程: https://stdcxx.apache.org/doc/stdlibug/11-3.html

## WTF

easy

- process 开始后，不可改变目标点？ ok

- 决策函数解耦
- 代价函数在 Simulation 层中
- Context
- Simulation

  - 状态
  - 下一个 event
  - 各个站点的 action
    - ActionQuery 队列？
    - process_delay_and_send_to()
      - assert(该边存在)
      - 吞吐量：单位时间这个站点能处理的包包
      - push 事件到事件堆 => 现在， { id, from, to }
      - push [ process_time (0.1s) 之后, START_DELAY { id, from, to } ]
      - push [ delay(2s) 之后，START_SEND { id, from, to } ]
      - 对于任意 DELAY 触发时，push [ trip_time(5s) 之后, ARRIVE { id, to } ]
    - 站点发现 ARRIVE s.t. to 为自己时，更新自己的状态 (unprocessed)
    - struct Site { property ,unprocessed: list<Pack> }

```cpp
// in main
// 所有包裹就是通过该 heap 实现
// Dicision 不可获取 heap
priority_queue<Event> event_heap;
// in main
queue<Request> requests;

real_world.step(); // thers is machine in real_world
```

中转站处理 Request => 产生新 event
time 可以 march 到下一个 event 了吗？
发送 event 记录到 Decision
Decision 发起 Request
Decision 发起 ok

```cpp
struct RealWorld {
    double time;
    map<string, PackageInfo> real_package_info;
    priority_queue<Event> real_events;
    queue<Request> requests_to_workers;
    DecisionTrait& decider;

    void add_package_task() {

    }
    void loop() {
        while (true) {
            march_to_next_real_event();
            this.decider.handle_events(requests_to_workers);
        }
    }
    void march_to_next_real_event() {
        assert();
        this->time =
        decider.add_event();
    }
    void deal_with_requests(queue<Request>& requests) {
    }
};

struct PackageInfo {

};

struct PackgeState {

};


struct Site {
    property,
    unprocessed_id: list<string>,
};
```

- Decision

  - 自行构建决策状态 class

```cpp
struct DecisionTrait {
    void push_request(queue<Request>& requests) const {
        requests.push(
            Request {
                processor_station: "c1",
                id: "a123-4556",
                to: "s12",
            }
        );
    }
};

struct EasiestDecision {
    priority_queue<Event> incomming_events; // who pop?
    map<string, PackageInfo> package_info;
};

struct Pack
```

- 简单模型：只有两个站点

```cpp
struct Graph {
    vector<Road> roads;
    vector<Station> stations;
    vector<Package> packages;
} graph;

struct Road {

};

struct Station {

};

struct Package {

};
```
