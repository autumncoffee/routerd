#pragma once

#include <ac-library/http/handler/handler.hpp>
#include <routerd_lib/structs.hpp>
#include <routerd_lib/request.hpp>
#include <utility>
#include <unordered_map>
#include <ac-library/http/server/await_client.hpp>
#include <ac-library/http/abstract_message.hpp>

namespace NAC {
    class TRouterDProxyHandler : public NHTTPHandler::THandler {
    public:
        TRouterDProxyHandler(
            const std::unordered_map<std::string, std::vector<TServiceHost>>& hosts,
            TRouterDGraph&& graph
        )
            : NHTTPHandler::THandler()
            , Hosts(hosts)
            , Graph(std::move(graph))
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
        void Iter(std::shared_ptr<TRouterDRequest> request) const;
        void ServiceReplied(std::shared_ptr<TRouterDRequest> request, const std::string& serviceName) const;
        void ProcessServiceResponse(
            std::shared_ptr<TRouterDRequest> request,
            std::shared_ptr<NHTTP::TIncomingResponse> response,
            const std::string& serviceName,
            const NHTTP::TAbstractMessage* part,
            bool contentDispositionFormData = true
        ) const;

    private:
        const std::unordered_map<std::string, std::vector<TServiceHost>>& Hosts;
        TRouterDGraph Graph;
    };
}
