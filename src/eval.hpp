#ifndef EVAL_HPP
#define EVAL_HPP

#include <map>
#include <string>

#include "base.hpp"
#include "fmt/core.h"
#include "log.hpp"

namespace eval {
using log::logs;
using log::logs_cargo;
using std::map;
using std::string;
using namespace base;

enum struct EvaluateVersion {
    V0 = 0,
    V1,
    V2,
};

struct EvalFunc {
    virtual double operator()(double transport_cost, const map<string, Package>& pkg) = 0;
};

struct EvalFuncV0: public EvalFunc {
    double operator()(double transport_cost, const map<string, Package>& pkgs) override {
        double tot_cost = transport_cost;
        logs_cargo("Evaluate", "transport cost: {}", transport_cost);
        for (const auto& [id, pkg]: pkgs) {
            if (!pkgs.at(id).finished) {
                tot_cost += 1e6;
                logs_cargo("Evaluate", "package {} not finished", id);
                continue;
            }
            double time_cost = pkgs.at(id).time_finished - pkg.time_created;
            double cost = time_cost * (pkg.category == PackageCategory::EXPRESS ? 10 : 5);
            logs_cargo(
                "Evaluate",
                "package {} finished at {}, cost {}",
                id,
                pkgs.at(id).time_finished,
                cost
            );
            tot_cost += cost;
        }
        return tot_cost;
    }
};

const std::pair<EvaluateVersion, EvalFunc*> EVALUATE_FUNC_MAP[] = {
    { EvaluateVersion::V0, new EvalFuncV0 },
    // { EvaluateVersion::V1,
    //   new EvalFuncV0 }
};

} // namespace eval
#endif
