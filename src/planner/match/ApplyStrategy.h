/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef PLANNER_MATCH_APPLYSTRATEGY_H_
#define PLANNER_MATCH_APPLYSTRATEGY_H_

#include "planner/PlanNode.h"
#include "planner/match/SegmentsConnector.h"

namespace nebula {
namespace graph {
/*
 * The ApplyStrategy was designed to connect two subplan by adding dependency.
 */
class ApplyStrategy final : public SegmentsConnectStrategy {
public:
    explicit ApplyStrategy(QueryContext *qctx) : SegmentsConnectStrategy(qctx) {}

    PlanNode* connect(const PlanNode* left, const PlanNode* right) override;
};
}   // namespace graph
}   // namespace nebula
#endif   // PLANNER_MATCH_SIMPLECONNECTSTRATEGY_H_