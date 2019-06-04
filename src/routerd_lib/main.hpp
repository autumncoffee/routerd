#pragma once

#include <string>
#include <ac-library/http/server/client.hpp>

namespace NAC {
    int RouterDMain(const std::string& configPath);

    int RouterDMain(
        const std::string& configPath,
        NHTTPServer::TClient::TArgs::TRequestFactory&& requestFactory
    );
}
