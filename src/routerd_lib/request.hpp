#pragma once

#include <ac-library/http/request.hpp>
#include <ac-common/string_sequence.hpp>
#include <utility>
#include <json.hh>

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

        TBlobSequence OutgoingRequest();

        void NextStage() {
            ++Stage;
            Counter = 0;
        }

        size_t GetStage() const {
            return Stage;
        }

        void NewReply() {
            ++Counter;
        }

        size_t ReplyCount() const {
            return Counter;
        }

    private:
        TArgs Args;
        bool OutgoingRequestInited = false;
        NHTTP::TResponse OutgoingRequest_;
        size_t Stage = 0;
        size_t Counter = 0;
    };
}
