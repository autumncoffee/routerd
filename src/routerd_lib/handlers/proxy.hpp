#pragma once

#include <ac-library/http/handler/handler.hpp>
#include <routerd_lib/structs.hpp>
#include <routerd_lib/request.hpp>
#include <utility>
#include <unordered_map>
#include <ac-library/http/server/await_client.hpp>
#include <ac-library/http/abstract_message.hpp>
#include <memory>

namespace NAC {
    class TStatWriter;

    class TRouterDProxyHandler : public NHTTPHandler::THandler {
    public:
        struct TArgs {
            const std::unordered_map<std::string, std::vector<TServiceHost>>& Hosts;
            TRouterDGraph Graph;
        };

    public:
        TRouterDProxyHandler(const TArgs& args, std::shared_ptr<TStatWriter> statWriter)
            : NHTTPHandler::THandler()
            , Hosts(args.Hosts)
            , Graph(args.Graph)
            , StatWriter(statWriter)
        {
        }

        void Handle(
            const std::shared_ptr<TRouterDRequest> request,
            const std::vector<std::string>& args
        );

        void Handle(
            const std::shared_ptr<NHTTP::TRequest> request,
            const std::vector<std::string>& args
        ) override {
            Handle(std::shared_ptr<TRouterDRequest>(request, (TRouterDRequest*)request.get()), args);
        }

    private:
        const TServiceHost& GetHost(const std::string& service) const;
        void Iter(std::shared_ptr<TRouterDRequest> request, const std::vector<std::string>& args) const;
        void ServiceReplied(std::shared_ptr<TRouterDRequest> request, const std::string& serviceName) const;
        void ProcessServiceResponse(
            std::shared_ptr<TRouterDRequest> request,
            std::shared_ptr<NHTTP::TIncomingResponse> response,
            const std::string& serviceName,
            const NHTTP::TAbstractMessage* part,
            bool contentDispositionFormData = true
        ) const;
#ifdef AC_DEBUG_ROUTERD_PROXY
        void print_outgoing_request(std::shared_ptr<TRouterDRequest> request) const;
#endif

    private:
        const std::unordered_map<std::string, std::vector<TServiceHost>>& Hosts;
        TRouterDGraph Graph;
        std::shared_ptr<TStatWriter> StatWriter;
    };
}
