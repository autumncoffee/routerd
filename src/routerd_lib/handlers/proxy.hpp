#pragma once

#include <ac-library/http/handler/handler.hpp>
#include <routerd_lib/request.hpp>
#include <utility>
#include <unordered_map>

namespace NAC {
    struct TServiceHost {
        std::string Addr;
        unsigned short Port = 0;
    };

    class TRouterDProxyHandler : public NHTTPHandler::THandler {
    public:
        TRouterDProxyHandler(
            const std::unordered_map<std::string, std::vector<TServiceHost>>& hosts,
            std::vector<std::vector<std::string>>&& order
        )
            : NHTTPHandler::THandler()
            , Hosts(hosts)
            , Order(std::move(order))
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
        void OnStageDone(std::shared_ptr<TRouterDRequest> request) const;

    private:
        const std::unordered_map<std::string, std::vector<TServiceHost>>& Hosts;
        const std::vector<std::vector<std::string>> Order;
    };
}
