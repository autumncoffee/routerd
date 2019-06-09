#pragma once

#include <string>
#include <ac-library/http/server/client.hpp>
#include <json.hh>

namespace NAC {
    using TRouterDRequestFactoryFactory = std::function<NHTTPServer::TClient::TArgs::TRequestFactory(
        const nlohmann::json&
    )>;

    TRouterDRequestFactoryFactory DefaultRouterDRequestFactoryFactory();

    int RouterDMain(const std::string& configPath);

    int RouterDMain(
        const std::string& configPath,
        TRouterDRequestFactoryFactory&& requestFactory
    );
}
