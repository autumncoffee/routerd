#pragma once

#include <ac-library/http/handler/handler.hpp>
#include <routerd_lib/stat.hpp>
#include <vector>
#include <memory>

namespace NAC {
    class TRouterDStatHandler : public NHTTPHandler::THandler {
    public:
        TRouterDStatHandler(std::vector<std::shared_ptr<TStatWriter>>& stats)
            : NHTTPHandler::THandler()
            , Stats(stats)
        {
        }

        void Handle(
            const std::shared_ptr<NHTTP::TRequest> request,
            const std::vector<std::string>& args
        ) override;

    private:
        std::vector<std::shared_ptr<TStatWriter>>& Stats;
    };
}
