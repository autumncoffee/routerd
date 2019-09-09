#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace NAC {
    struct TServiceHost {
        std::string Addr;
        unsigned short Port = 0;
    };

    struct TService {
        std::string Name;
        std::string HostsFrom;
        std::string Path;
    };

    class TStatWriter;

    struct TRouterDGraph {
        using TTree = std::unordered_map<std::string, std::unordered_set<std::string>>;

        std::unordered_map<std::string, TService> Services;
        TTree Tree;
        TTree ReverseTree;

        std::shared_ptr<TStatWriter> StatWriter;
    };
}
