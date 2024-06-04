#ifndef EVAL_HPP
#define EVAL_HPP

#include <cwchar>
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
        logs_cargo("Evaluate", "system transport cost: {}", transport_cost);
        for (const auto& [id, pkg]: pkgs) {
            if (!pkgs.at(id).finished) {
                tot_cost += 1e6;
                logs_cargo("Evaluate", "package {} not finished", id);
                continue;
            }
            double time_spent = pkgs.at(id).time_finished - pkg.time_created;
            double cost = time_spent * (pkg.category == PackageCategory::EXPRESS ? 3 : 1);
            logs_cargo(
                "Evaluate",
                "package {} type {} finished at {:.2f}, spent {:.2f} cost {:.2f}",
                id,
                pkg.category == PackageCategory::EXPRESS ? "express" : "normal",
                pkgs.at(id).time_finished,
                time_spent,
                cost
            );
            tot_cost += cost;
        }
        return tot_cost;
    }
};

struct EvalFuncV1: public EvalFunc {
    double operator()(double transport_cost, const map<string, Package>& pkgs) override;
};

const std::pair<EvaluateVersion, EvalFunc*> EVALUATE_FUNC_MAP[] = {
    { EvaluateVersion::V0, new EvalFuncV0 },
    { EvaluateVersion::V1, new EvalFuncV1 },
    // { EvaluateVersion::V1,
    //   new EvalFuncV0 }
};

const double V1_EXPRESS_DDL_HOURS = 24;
const double V1_STANDARD_DDL_HOURS = 72;
const double OVER_DDL_PUNISHMENT = 50;
const double OVER_DDL_PUNISHMENT_PER_HOUR = 3;
const double NON_DDL_COST_PER_HOUR = 3;

inline double EvalFuncV1::operator()(double transport_cost, const map<string, Package>& pkgs) {
    double tot_cost = transport_cost;
    logs_cargo("Evaluate", "system transport cost: {}", transport_cost);
    for (const auto& [id, pkg]: pkgs) {
        if (!pkgs.at(id).finished) {
            tot_cost += 1e6;
            logs_cargo("Evaluate", "package {} not finished", id);
            continue;
        }
        // 运输成本
        // EXPRESS 包裹运输时间 * 1，若超过 24h，立即增加 50，每小时增加 3
        // STANDARD 包裹运输时间 * 1，若超过 72h，立即增加 50，每小时增加 3
        const double time_spent = pkgs.at(id).time_finished - pkg.time_created;
        const double ddl =
            pkg.category == PackageCategory::EXPRESS ? V1_EXPRESS_DDL_HOURS : V1_STANDARD_DDL_HOURS;
        const double cost = time_spent > ddl ? OVER_DDL_PUNISHMENT
                + (time_spent - ddl) * OVER_DDL_PUNISHMENT_PER_HOUR + ddl * NON_DDL_COST_PER_HOUR
                                             : time_spent * NON_DDL_COST_PER_HOUR;
        tot_cost += cost;
    }
    return tot_cost;
}
} // namespace eval
#endif
