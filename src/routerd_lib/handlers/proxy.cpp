#include "proxy.hpp"
#include <stdlib.h>
#include <ac-common/str.hpp>
#include <utility>
#include <random>
#include <routerd_lib/utils.hpp>
#include <ac-library/http/server/await_client.hpp>

#ifdef AC_DEBUG_ROUTERD_PROXY
#include <iostream>
#endif

namespace NAC {
    void TRouterDProxyHandler::Handle(
        std::shared_ptr<TRouterDRequest> request,
        const std::vector<std::string>& /* args */
    ) {
#ifdef AC_DEBUG_ROUTERD_PROXY
        for (const auto& it1 : Graph.Tree) {
            std::cerr << it1.first << " depends on:" << std::endl;

            for (const auto& it2 : it1.second) {
                std::cerr << "\t" << it2 << std::endl;
            }
        }
#endif

        request->SetGraph(Graph);

        Iter(request);
    }

    const TServiceHost& TRouterDProxyHandler::GetHost(const std::string& service) const {
        const auto& hosts = Hosts.at(service);

        if (hosts.size() > 1) {
            thread_local static std::random_device rd;
            thread_local static std::mt19937 g(rd());
            std::uniform_int_distribution<size_t> dis(0, hosts.size() - 1);

            return hosts.at(dis(g));
        }

        return hosts.front();
    }

    void TRouterDProxyHandler::Iter(std::shared_ptr<TRouterDRequest> request) const {
        const auto& graph = request->GetGraph();

        for (auto&& treeIt : graph.Tree) {
            if (!treeIt.second.empty() || request->IsInProgress(treeIt.first)) {
                continue;
            }

#ifdef AC_DEBUG_ROUTERD_PROXY
            std::cerr << "graph.Services.at(" << treeIt.first << ");" << std::endl;
#endif

            const auto& service = graph.Services.at(treeIt.first);
            const auto& host = GetHost(service.HostsFrom);
            auto rv = request->AwaitHTTP(host.Addr.c_str(), host.Port, [this, request, &service](
                std::shared_ptr<NHTTP::TIncomingResponse> response,
                std::shared_ptr<NHTTPServer::TClientBase> client
            ) {
                client->Drop(); // TODO
                request->NewReply(service.Name);

                ServiceReplied(request, service.Name);

                if (service.Name == std::string("output")) { // TODO
                    NHTTP::TResponse out;
                    out.FirstLine(response->FirstLine() + "\r\n");
                    CopyHeaders(response->Headers(), out);
                    out.Wrap(response->ContentLength(), response->Content());
                    out.Memorize(response);

                    request->Send(out);
                }

                {
                    auto part = request->PreparePart(service.Name);
                    part.Wrap(response->ContentLength(), response->Content());
                    part.Memorize(response);
                    request->AddPart(std::move(part));
                }

                const auto& graph = request->GetGraph();

                if (graph.Tree.empty()) {
                    if ((request->InProgressCount() == 0) && !request->IsResponseSent()) {
                        request->Send500();
                    }

                } else {
                    Iter(request);
                }
            });

            if (!rv) {
                ServiceReplied(request, service.Name);
                continue;
            }

            auto msg = request->OutgoingRequest(service.Path);
            msg.Memorize(request);

            request->NewRequest(service.Name);
            rv->PushWriteQueueData(msg);
        }

        if ((request->InProgressCount() == 0) && graph.Tree.empty() && !request->IsResponseSent()) {
            request->Send500();
        }
    }

    void TRouterDProxyHandler::ServiceReplied(std::shared_ptr<TRouterDRequest> request, const std::string& serviceName) const {
        auto&& graph = request->GetGraph();
        const auto& it1 = graph.ReverseTree.find(serviceName);

        if (it1 != graph.ReverseTree.end()) {
            for (const auto& it2 : it1->second) {
#ifdef AC_DEBUG_ROUTERD_PROXY
                std::cerr << "graph.Tree.at(" << it2 << ").erase(" << it1->first << ");" << std::endl;
#endif

                graph.Tree.at(it2).erase(it1->first);
            }

#ifdef AC_DEBUG_ROUTERD_PROXY
            std::cerr << "graph.ReverseTree.erase(" << it1->first << ");" << std::endl;
#endif

            graph.ReverseTree.erase(it1);
        }

#ifdef AC_DEBUG_ROUTERD_PROXY
        std::cerr << "graph.Tree.erase(" << serviceName << ");" << std::endl;
#endif

        graph.Tree.erase(serviceName);
    }
}
