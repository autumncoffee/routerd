#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace NAC {
    struct TServiceHost {
        std::string Addr;
        unsigned short Port = 0;
    };

    struct TService {
        std::string Name;
        std::string HostsFrom;
        std::string Path;
        std::string SendRawOutputOf;
        std::string SaveAs;
    };

    struct TRouterDGraph {
        using TTree = std::unordered_map<std::string, std::unordered_set<std::string>>;

        std::unordered_map<std::string, TService> Services;
        TTree Tree;
        TTree ReverseTree;
    };
}
