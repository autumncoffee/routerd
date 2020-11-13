#pragma once

#include <ac-library/http/request.hpp>
#include <ac-common/string_sequence.hpp>
#include <utility>
#include <json.hh>
#include "structs.hpp"
#include <unordered_set>
#include <chrono>

namespace NAC {
    class TRouterDRequest : public NHTTP::TRequest {
    public:
        struct TArgs {
            bool AllowNestedRequests = false;

            static TArgs FromConfig(const nlohmann::json&);
        };

    public:
        template<typename... TArgs_>
        TRouterDRequest(const TArgs& args, TArgs_&&... args_)
            : NHTTP::TRequest(std::forward<TArgs_>(args_)...)
            , Args(args)
            , StartTime_(std::chrono::steady_clock::now())
        {
        }

    protected:
        virtual void PrepareOutgoingRequest(NHTTP::TResponse&) {
        }

    private:
        NHTTP::TResponse& Out();

    public:
        NHTTP::TResponse PreparePart(const std::string& partName) const;
        void AddPart(NHTTP::TResponse&& part);

        virtual const std::string& DefaultChunkName() const {
            static const std::string defaultChunkName("default");

            return defaultChunkName;
        }

        TBlobSequence OutgoingRequest(const std::string& path, const std::vector<std::string>& args);

        void SetGraph(const TRouterDGraph& graph) {
            Graph = graph;
        }

        const TRouterDGraph& GetGraph() const {
            return Graph;
        }

        TRouterDGraph& GetGraph() {
            return Graph;
        }

        void NewReply(const std::string& name) {
            InProgress.erase(name);
        }

        void NewRequest(const std::string& name) {
            InProgress.insert(name);
        }

        size_t InProgressCount() const {
            return InProgress.size();
        }

        bool IsInProgress(const std::string& name) const {
            return (InProgress.count(name) > 0);
        }

        const NHTTP::TResponse& GetOutGoingRequest() {
            return Out();
        }

        const std::chrono::steady_clock::time_point& StartTime() const {
            return StartTime_;
        }

    private:
        TArgs Args;
        bool OutgoingRequestInited = false;
        NHTTP::TResponse OutgoingRequest_;
        TRouterDGraph Graph;
        std::unordered_set<std::string> InProgress;
        std::chrono::steady_clock::time_point StartTime_;
    };
}
