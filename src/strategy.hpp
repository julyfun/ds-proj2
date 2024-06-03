#ifndef STRATEGY_HPP
#define STRATEGY_HPP

#include <map>
#include <queue>
#include <utility>
#include <vector>

#include "base.hpp"

namespace strategy {
using namespace base;
using std::greater;
using std::map;
using std::pair;
using std::priority_queue;
using std::vector;

enum struct StrategyVersion {
    V0 = 0,
    V1,
    V1A,
    V2,
};

// 使用堆优化 dijkstra 求解时间最短路，返回最短路整条路径 id vector
// routes[x] is all routes of x
// routes[x][rid] is route of x
inline vector<int> dijkstra(
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
            // 不知道是什么种类的包裹
            // 包含站点处理价格
            double w =
                route.second.time * 7.5 + route.second.cost + stations.at(route.second.dst).cost;
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

} // namespace strategy

#endif
