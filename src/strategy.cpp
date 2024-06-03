#include "doctest/doctest.h"

#include "strategy.hpp"

namespace strategy {

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
        { 2, Route { 2, "a", "c", 2, 1 } },
    };
    routes["b"] = {
        { 3, Route { 3, "b", "c", 3, 1 } },
        { 4, Route { 4, "b", "d", 4, 1 } },
    };
    routes["c"] = {
        { 5, Route { 5, "c", "d", 5, 1 } },
    };
    routes["d"] = {
        { 6, Route { 6, "d", "e", 6, 1 } },
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

// 第 7 条路径的代价特别大
TEST_CASE("dijkstra2") {
    auto stations = map<string, Station> {
        { "a", Station { "a", 1.0 / 6, 2, 0 } }, { "b", Station { "b", 1.0 / 5, 2, 0 } },
        { "c", Station { "c", 1.0 / 4, 2, 0 } }, { "d", Station { "d", 1.0 / 3, 2, 0 } },
        { "e", Station { "e", 1.0 / 2, 2, 0 } },
    };
    map<string, map<int, Route>> routes;
    routes["a"] = {
        { 1, Route { 1, "a", "b", 1, 1 } },
        { 7, Route { 7, "a", "b", 0.9, 100 } },
        { 8, Route { 8, "a", "b", 2, 1 } },
        { 2, Route { 2, "a", "c", 2, 1 } },
    };
    routes["b"] = {
        { 3, Route { 3, "b", "c", 3, 1 } },
        { 4, Route { 4, "b", "d", 4, 1 } },
    };
    routes["c"] = {
        { 5, Route { 5, "c", "d", 5, 1 } },
    };
    routes["d"] = {
        { 6, Route { 6, "d", "e", 6, 1 } },
    };
    auto path = dijkstra(stations, routes, "a", "e");
    // for (const auto& station: path) {
    //     cout << station << " ";
    // }
    // cout << '\n';
    auto ans = vector<int> { 1, 4, 6 };
    for (int i = 0; i < path.size(); i++) {
        CHECK(path[i] == ans[i]);
    }
}

// 到达 b 站点的收费特别大
TEST_CASE("dijkstra3") {
    auto stations = map<string, Station> {
        { "a", Station { "a", 1.0 / 6, 2, 0 } }, { "b", Station { "b", 1.0 / 5, 2, 100 } },
        { "c", Station { "c", 1.0 / 4, 2, 0 } }, { "d", Station { "d", 1.0 / 3, 2, 0 } },
        { "e", Station { "e", 1.0 / 2, 2, 0 } },
    };
    map<string, map<int, Route>> routes;
    routes["a"] = {
        { 1, Route { 1, "a", "b", 1, 1 } },
        { 7, Route { 7, "a", "b", 0.9, 1 } },
        { 8, Route { 8, "a", "b", 2, 1 } },
        { 2, Route { 2, "a", "c", 2, 1 } },
    };
    routes["b"] = {
        { 3, Route { 3, "b", "c", 3, 1 } },
        { 4, Route { 4, "b", "d", 4, 1 } },
    };
    routes["c"] = {
        { 5, Route { 5, "c", "d", 5, 1 } },
    };
    routes["d"] = {
        { 6, Route { 6, "d", "e", 6, 1 } },
    };
    auto path = dijkstra(stations, routes, "a", "e");
    // for (const auto& station: path) {
    //     cout << station << " ";
    // }
    // cout << '\n';
    auto ans = vector<int> { 2, 5, 6 };
    for (int i = 0; i < path.size(); i++) {
        CHECK(path[i] == ans[i]);
    }
}

// 到达 b 站点的 process_delay 特别大
TEST_CASE("dijkstra4") {
    auto stations = map<string, Station> {
        { "a", Station { "a", 1.0 / 6, 2, 0 } }, { "b", Station { "b", 1.0 / 5, 100, 0 } },
        { "c", Station { "c", 1.0 / 4, 2, 0 } }, { "d", Station { "d", 1.0 / 3, 2, 0 } },
        { "e", Station { "e", 1.0 / 2, 2, 0 } },
    };
    map<string, map<int, Route>> routes;
    routes["a"] = {
        { 1, Route { 1, "a", "b", 1, 1 } },
        { 7, Route { 7, "a", "b", 0.9, 1 } },
        { 8, Route { 8, "a", "b", 2, 1 } },
        { 2, Route { 2, "a", "c", 2, 1 } },
    };
    routes["b"] = {
        { 3, Route { 3, "b", "c", 3, 1 } },
        { 4, Route { 4, "b", "d", 4, 1 } },
    };
    routes["c"] = {
        { 5, Route { 5, "c", "d", 5, 1 } },
    };
    routes["d"] = {
        { 6, Route { 6, "d", "e", 6, 1 } },
    };
    auto path = dijkstra(stations, routes, "a", "e");
    // for (const auto& station: path) {
    //     cout << station << " ";
    // }
    // cout << '\n';
    auto ans = vector<int> { 2, 5, 6 };
    for (int i = 0; i < path.size(); i++) {
        CHECK(path[i] == ans[i]);
    }
}

TEST_CASE("dijkstra-v1b") {
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
        { 2, Route { 2, "a", "c", 2, 1 } },
    };
    routes["b"] = {
        { 3, Route { 3, "b", "c", 3, 1 } },
        { 4, Route { 4, "b", "d", 4, 1 } },
    };
    routes["c"] = {
        { 5, Route { 5, "c", "d", 5, 1 } },
    };
    routes["d"] = {
        { 6, Route { 6, "d", "e", 6, 1 } },
    };
    stations.at("b").buffer.emplace("omg");
    auto path = dijkstra_enhanced(stations, routes, "a", "e");
    // for (const auto& station: path) {
    //     cout << station << " ";
    // }
    // cout << '\n';
    auto ans = vector<int> { 2, 5, 6 };
    for (int i = 0; i < path.size(); i++) {
        CHECK(path[i] == ans[i]);
    }
}

// 极小的 throughput，需要调用 dijkstra
TEST_CASE("dijkstra-v1b-2") {
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
        { 2, Route { 2, "a", "c", 2, 1 } },
    };
    routes["b"] = {
        { 3, Route { 3, "b", "c", 3, 1 } },
        { 4, Route { 4, "b", "d", 4, 1 } },
    };
    routes["c"] = {
        { 5, Route { 5, "c", "d", 5, 1 } },
    };
    routes["d"] = {
        { 6, Route { 6, "d", "e", 6, 1 } },
    };
    stations.at("b").buffer.emplace("omg");
    stations.at("c").buffer.emplace("omg");
    auto path = dijkstra_enhanced(stations, routes, "a", "e");
    // for (const auto& station: path) {
    //     cout << station << " ";
    // }
    // cout << '\n';
    auto ans = vector<int> { 7, 4, 6 };
    for (int i = 0; i < path.size(); i++) {
        CHECK(path[i] == ans[i]);
    }
}

} // namespace strategy
