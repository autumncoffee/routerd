#include "proxy.hpp"
#include <stdlib.h>
#include <ac-common/str.hpp>
#include <utility>
#include <random>
#include <routerd_lib/utils.hpp>
#include <routerd_lib/stat.hpp>
#include <ac-common/utils/string.hpp>

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
        auto&& graph = request->GetGraph();

        while (true) {
            bool somethingHappened(false);
            std::vector<std::string> failedServices;

            for (auto&& treeIt : graph.Tree) {
                if (!treeIt.second.empty() || request->IsInProgress(treeIt.first)) {
                    continue;
                }

                somethingHappened = true;

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

                    if (response->ContentType() == std::string("multipart/x-ac-routerd")) {
                        bool haveDirectResponse(false);

                        for (const auto& part : response->Parts()) {
                            std::string partName;
                            NStringUtils::Strip(part.ContentDispositionParams().at("filename"), partName, 2, "\"'");

                            if (partName == service.Name) {
                                haveDirectResponse = true;
                            }

                            ProcessServiceResponse(request, response, partName, &part, /* contentDispositionFormData = */false);
                        }

                        if (!haveDirectResponse) {
                            ServiceReplied(request, service.Name);
                        }

                    } else {
                        ProcessServiceResponse(request, response, service.Name, response.get());
                    }

                    Iter(request);
                });

                if (!rv) {
                    failedServices.push_back(service.Name);
                    continue;
                }

                auto msg = request->OutgoingRequest(service.Path);
                msg.Memorize(request);

                request->NewRequest(service.Name);
                rv->PushWriteQueueData(msg);
            }

            for (const auto& name : failedServices) {
                graph.Tree.erase(name);
            }

            if (request->InProgressCount() == 0) { // if we couldn't send any requests
                if (somethingHappened) { // but tried to
                    if (graph.Tree.empty()) { // and there are no services left
                        if (!request->IsResponseSent()) {
                            request->Send500();
                        }

                    } else { // and still have services to try
                        continue;
                    }

                } else { // and won't send any
                    if (!request->IsResponseSent()) {
                        request->Send500();
                    }
                }
            }

            break;
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

    void TRouterDProxyHandler::ProcessServiceResponse(
        std::shared_ptr<TRouterDRequest> request,
        std::shared_ptr<NHTTP::TIncomingResponse> response,
        const std::string& serviceName,
        const NHTTP::TAbstractMessage* message,
        bool contentDispositionFormData
    ) const {
        ServiceReplied(request, serviceName);

        if ((serviceName == std::string("output")) && !request->IsResponseSent()) { // TODO
            {
                NHTTP::TResponse out;
                out.FirstLine(response->FirstLine() + "\r\n");
                CopyHeaders(message->Headers(), out, /* contentType = */true, contentDispositionFormData);
                out.Wrap(message->ContentLength(), message->Content());
                out.Memorize(response);

                request->Send(out);
            }

            {
                size_t statusCode = response->StatusCode();
                const auto& statusCodeHint = message->HeaderValue("x-ac-routerd-statuscode");

                if (!statusCodeHint.empty()) {
                    NStringUtils::FromString(statusCodeHint, statusCode);
                }

                TStatReport report;
                report.OutputStatusCode = statusCode;
                StatWriter->Write(report);
            }
        }

        {
            auto part = request->PreparePart(serviceName);
            part.Wrap(message->ContentLength(), message->Content());
            part.Memorize(response);
            request->AddPart(std::move(part));
        }
    }
}
