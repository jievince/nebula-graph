/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef EXECUTOR_STORAGEACCESSEXECUTOR_H_
#define EXECUTOR_STORAGEACCESSEXECUTOR_H_

#include <thrift/lib/cpp/util/EnumUtils.h>
#include "common/clients/storage/StorageClientBase.h"
#include "context/QueryContext.h"
#include "executor/Executor.h"
#include "service/Session.h"

namespace nebula {

class Expression;

namespace graph {

class Iterator;
struct SpaceInfo;

// It's used for data write/update/query
class StorageAccessExecutor : public Executor {
protected:
    StorageAccessExecutor(const std::string &name, const PlanNode *node, QueryContext *qctx)
        : Executor(name, node, qctx) {}

    // Parameter isPartialSuccessAccepted to specify
    // whether to return an error for partial succeeded.
    // An error will be returned if isPartialSuccessAccepted
    // is set to false and completeness is less than 100.
    template <typename Resp>
    StatusOr<Result::State> handleCompleteness(const storage::StorageRpcResponse<Resp> &rpcResp,
                                               bool isPartialSuccessAccepted) const {
        auto completeness = rpcResp.completeness();
        if (completeness != 100) {
            const auto &failedCodes = rpcResp.failedParts();
            for (auto it = failedCodes.begin(); it != failedCodes.end(); it++) {
                LOG(ERROR)
                    << name_ << " failed, error "
                    << "fuck"
                    // <<
                    // std::to_string(static_cast<std::underlying_type<nebula::storage::ErrorCode>::type>(it->second))
                    // << apache::thrift::util::enumNameSafe(it->second)
                    << ", part " << it->first;
            }
            // cannot execute at all, or partial success is not accepted
            if (completeness == 0 || !isPartialSuccessAccepted) {
                if (failedCodes.size() > 0) {
                    return handleErrorCode(failedCodes.begin()->second, failedCodes.begin()->first);
                }
                return Status::Error("Request to storage failed, without failedCodes.");
            }
            // partial success is accepted
            qctx()->setPartialSuccess();
            return Result::State::kPartialSuccess;
        }
        return Result::State::kSuccess;
    }

    Status handleErrorCode(nebula::storage::ErrorCode code, PartitionID partId) const {
        switch (code) {
            case storage::ErrorCode::E_KEY_NOT_FOUND:
                return Status::Error("Storage Error: Vertex or edge not found.");
            case storage::ErrorCode::E_DATA_TYPE_MISMATCH: {
                std::string error = "Storage Error: The data type does not meet the requirements. "
                                    "Use the correct type of data.";
                return Status::Error(std::move(error));
            }
            case storage::ErrorCode::E_INVALID_VID: {
                std::string error = "Storage Error: The VID must be a 64-bit interger"
                                    " or a string fitting space vertex id length limit.";
                return Status::Error(std::move(error));
            }
            case storage::ErrorCode::E_INVALID_FIELD_VALUE: {
                std::string error = "Storage Error: Invalid field value: "
                                    "may be the filed is not NULL "
                                    "or without default value or wrong schema.";
                return Status::Error(std::move(error));
            }
            case storage::ErrorCode::E_LEADER_CHANGED:
                return Status::Error("Storage Error: The leader has changed. Try again later");
            case storage::ErrorCode::E_INVALID_FILTER:
                return Status::Error("Storage Error: Invalid filter.");
            case storage::ErrorCode::E_INVALID_UPDATER:
                return Status::Error("Storage Error: Invalid Update col or yield col.");
            case storage::ErrorCode::E_INVALID_SPACEVIDLEN:
                return Status::Error("Storage Error: Invalid space vid len.");
            case storage::ErrorCode::E_SPACE_NOT_FOUND:
                return Status::Error("Storage Error: Space not found.");
            case storage::ErrorCode::E_TAG_NOT_FOUND:
                return Status::Error("Storage Error: Tag not found.");
            case storage::ErrorCode::E_TAG_PROP_NOT_FOUND:
                return Status::Error("Storage Error: Tag prop not found.");
            case storage::ErrorCode::E_EDGE_NOT_FOUND:
                return Status::Error("Storage Error: Edge not found.");
            case storage::ErrorCode::E_EDGE_PROP_NOT_FOUND:
                return Status::Error("Storage Error: Edge prop not found.");
            case storage::ErrorCode::E_INDEX_NOT_FOUND:
                return Status::Error("Storage Error: Index not found.");
            case storage::ErrorCode::E_INVALID_DATA:
                return Status::Error("Storage Error: Invalid data, may be wrong value type.");
            case storage::ErrorCode::E_NOT_NULLABLE:
                return Status::Error("Storage Error: The not null field cannot be null.");
            case storage::ErrorCode::E_FIELD_UNSET:
                return Status::Error("Storage Error: "
                                     "The not null field doesn't have a default value.");
            case storage::ErrorCode::E_OUT_OF_RANGE:
                return Status::Error("Storage Error: Out of range value.");
            case storage::ErrorCode::E_ATOMIC_OP_FAILED:
                return Status::Error("Storage Error: Atomic operation failed.");
            case storage::ErrorCode::E_FILTER_OUT:
                return Status::OK();
            default:
                auto status = Status::Error(
                    "Storage Error: part: %d, error: %s(%d).",
                    partId,
                    "fuck",
                    //static_cast<typename std::underlying_type<storage::ErrorCode>::type>(code),
                    // apache::thrift::util::enumNameSafe(code).c_str(),
                    static_cast<int32_t>(code));
                LOG(ERROR) << status;
                return status;
        }
        return Status::OK();
    }

    template <typename RESP>
    void addStats(RESP &resp, std::unordered_map<std::string, std::string> &stats) const {
        auto &hostLatency = resp.hostLatency();
        for (size_t i = 0; i < hostLatency.size(); ++i) {
            auto &info = hostLatency[i];
            stats.emplace(
                folly::stringPrintf("%s exec/total", std::get<0>(info).toString().c_str()),
                folly::stringPrintf("%d(us)/%d(us)", std::get<1>(info), std::get<2>(info)));
        }
    }

    bool isIntVidType(const SpaceInfo &space) const;

    DataSet buildRequestDataSetByVidType(Iterator *iter, Expression *expr, bool dedup);
};

}   // namespace graph
}   // namespace nebula

#endif   // EXECUTOR_STORAGEACCESSEXECUTOR_H_
