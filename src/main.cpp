#include <handlers/proxy.hpp>
#include <ac-library/http/server/server.hpp>
#include <ac-library/http/server/client.hpp>
#include <ac-library/http/router/router.hpp>
#include <stdlib.h>
#include <json.hh>
#include <iostream>
#include <ac-common/file.hpp>
#include <unordered_set>

using namespace NAC;

nlohmann::json ParseConfig(const char* path) {
    TFile configFile(path);

    if (!configFile.IsOK()) {
        std::cerr << "Failed to open " << path << std::endl;
        return nullptr;
    }

    return nlohmann::json::parse(configFile.Data(), configFile.Data() + configFile.Size());
}

int main(int argc, const char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " /path/to/config.json" << std::endl;
        return 1;
    }

    auto&& config = ParseConfig(argv[1]);
    const std::string bind4((config.count("bind4") > 0) ? config["bind4"].get<std::string>() : "");
    const std::string bind6((config.count("bind6") > 0) ? config["bind6"].get<std::string>() : "");
    std::unordered_map<std::string, std::vector<TServiceHost>> hosts;

    for (const auto& spec : config["hosts"].get<std::unordered_map<std::string, std::vector<std::string>>>()) {
        if (spec.second.empty()) {
            std::cerr << spec.first << " has no hosts" << std::endl;
            return 1;
        }

        auto&& hosts_ = hosts[spec.first];
        hosts_.reserve(spec.second.size());

        for (const auto& host : spec.second) {
            const ssize_t colon(host.rfind(':'));

            if (colon < 0) {
                std::cerr << spec.first << ": " << host << " has no port specified" << std::endl;
                return 1;
            }

            std::stringstream ss;
            unsigned short port;
            ss << host.data() + colon + 1;
            ss >> port;

            hosts_.emplace_back(TServiceHost {
                .Addr = std::string(host.data(), colon),
                .Port = port
            });
        }
    }

    std::unordered_map<std::string, std::shared_ptr<NHTTPHandler::THandler>> graphs;

    for (const auto& graph : config["graphs"].get<std::unordered_map<std::string, nlohmann::json>>()) {
        std::unordered_set<std::string> services;
        const auto& data = graph.second;
        const auto& servicesVector = data["services"].get<std::vector<std::string>>();

        for (const auto& service : servicesVector) {
            if (hosts.count(service) == 0) {
                std::cerr << graph.first << ": unknown service: " << service << std::endl;
                return 1;
            }

            services.insert(service);
        }

        std::vector<std::vector<std::string>> order;

        if (data.count("deps") > 0) {
            std::unordered_map<std::string, std::unordered_set<std::string>> tree;
            std::unordered_map<std::string, std::unordered_set<std::string>> reverseTree;

            for (const auto& service : servicesVector) {
                tree.emplace(service, decltype(tree)::mapped_type());
            }

            for (const auto& dep : data["deps"].get<std::vector<nlohmann::json>>()) {
                const auto& a = dep["a"].get<std::string>();
                const auto& b = dep["b"].get<std::string>();

                if (a == b) {
                    std::cerr << graph.first << ": " << a << " depends on itself" << std::endl;
                    return 1;
                }

                if (services.count(a) == 0) {
                    std::cerr << graph.first << ": unknown service in dependency: " << a << std::endl;
                    return 1;
                }

                if (services.count(b) == 0) {
                    std::cerr << graph.first << ": unknown service in dependency: " << b << std::endl;
                    return 1;
                }

                tree[a].insert(b);
                reverseTree[b].insert(a);
            }

            while (!tree.empty()) {
                std::vector<std::string> noDeps;

                for (auto&& it1 : tree) {
                    if (!it1.second.empty()) {
                        continue;
                    }

                    noDeps.push_back(it1.first);

                    if (reverseTree.count(it1.first) > 0) {
                        for (const auto& it2 : reverseTree.at(it1.first)) {
                            tree.at(it2).erase(it1.first);
                        }

                        reverseTree.erase(it1.first);
                    }
                }

                if (noDeps.empty()) {
                    std::cerr << graph.first << ": cycle in dependencies" << std::endl;
                    return 1;
                }

                for (const auto& it : noDeps) {
                    tree.erase(it);
                }

                order.emplace_back(std::move(noDeps));
            }

        } else {
            order.push_back(servicesVector);
        }

        graphs.emplace(graph.first, std::make_shared<TRouterDProxyHandler>(hosts, std::move(order)));
    }

    NHTTPRouter::TRouter router;

    for (const auto& route : config["routes"].get<std::vector<nlohmann::json>>()) {
        router.Add(route["r"].get<std::string>(), graphs.at(route["g"].get<std::string>()));
    }

    NHTTPServer::TServer::TArgs args;

    args.BindIP4 = (bind4.empty() ? nullptr : bind4.c_str());
    args.BindIP6 = (bind6.empty() ? nullptr : bind6.c_str());
    args.BindPort6 = args.BindPort4 = config["port"].get<unsigned short>();
    args.ThreadCount = ((config.count("threads") > 0) ? config["threads"].get<size_t>() : 10);
    args.ClientArgsFactory = [&router]() {
        return new NHTTPServer::TClient::TArgs(router, [](
            std::shared_ptr<NHTTPLikeParser::TParsedData> data,
            const NHTTPServer::TResponder& responder
        ) {
            return (NHTTP::TRequest*)(new TRouterDRequest(data, responder));
        });
    };

    NHTTPServer::TServer(args, router).Run();

    return 0;
}
