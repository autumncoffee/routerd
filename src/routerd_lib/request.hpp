#pragma once

#include <ac-library/http/request.hpp>
#include <ac-common/string_sequence.hpp>
#include <utility>
#include <ac-library/http/utils/multipart.hpp>

namespace NAC {
    class TRouterDRequest : public NHTTP::TRequest {
    public:
        template<typename... TArgs>
        TRouterDRequest(TArgs&&... args)
            : NHTTP::TRequest(std::forward<TArgs>(args)...)
            , Boundary_(NHTTPUtils::Boundary())
        {
        }

        const std::string& Boundary() const {
            return Boundary_;
        }

    protected:
        virtual void PrepareOutgoingRequest(NHTTP::TResponse&) {
        }

    private:
        TBlobSequence& Data();
        void PushPartPreamble(const std::string& partName, size_t size);
        void PushDataImpl(const std::string& partName, TBlobSequence&& input);

    public:
        template<typename... TArgs>
        void PushData(const std::string& partName, TArgs&&... args) {
            PushDataImpl(partName, TBlobSequence::Construct(std::forward<TArgs>(args)...));
        }

        const std::string& DefaultChunkName() const {
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
        std::string Boundary_;
        bool DataInited = false;
        TBlobSequence Data_;
        bool OutgoingRequestInited = false;
        NHTTP::TResponse OutgoingRequest_;
        size_t Stage = 0;
        size_t Counter = 0;
    };
}
