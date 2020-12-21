/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "executor/query/UnwindExecutor.h"
#include "context/QueryExpressionContext.h"
#include "parser/Clauses.h"
#include "planner/Query.h"
#include "util/ScopedTimer.h"

namespace nebula {
namespace graph {

folly::Future<Status> UnwindExecutor::execute() {
    SCOPED_TIMER(&execTime_);

    auto *unwind = asNode<Unwind>(node());
    auto columns = unwind->columns()->columns();

    auto iter = ectx_->getResult(unwind->inputVar()).iter();
    DCHECK(!!iter);
    QueryExpressionContext ctx(ectx_);

    DataSet ds;
    ds.colNames = unwind->colNames();
    for (; iter->valid(); iter->next()) {
        Row row;
        for (auto& col : columns) {
            Value val = col->expr()->eval(ctx(iter.get()));
            std::vector<Value> vals = extractList(val);
            for (const auto &v : vals) {
                ds.rows.emplace_back(Row({std::move(v)}));
            }
        }
    }

    return finish(ResultBuilder().value(Value(std::move(ds))).finish());
}

std::vector<Value> UnwindExecutor::extractList(Value &val) {
    std::vector<Value> ret;
    if (val.isList()) {
        auto &list = val.getList();
        for (size_t i = 0; i < list.size(); ++i) {
            ret.emplace_back(std::move(list[i]));
        }
    } else {
        if (!(val.isNull() || val.empty())) {
            ret.emplace_back(std::move(val));
        }
    }

    return ret;
}

}   // namespace graph
}   // namespace nebula
