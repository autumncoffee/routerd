#include "proxy.hpp"
#include <stdlib.h>
#include <ac-common/str.hpp>
#include <utility>
#include <random>
#include <routerd_lib/utils.hpp>
#include <routerd_lib/stat.hpp>
#include <ac-common/utils/string.hpp>
#include <iostream>

namespace NAC {
    void TRouterDProxyHandler::Handle(
        std::shared_ptr<TRouterDRequest> request,
        const std::vector<std::string>& args
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

        Iter(request, args);
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

#ifdef AC_DEBUG_ROUTERD_PROXY
    void TRouterDProxyHandler::PrintOutgoingRequest(std::shared_ptr<TRouterDRequest> request) const{
        auto& outgoingRequest = request->GetOutGoingRequest();
        std::cerr << "== OUTGOING REQUEST == " << std::endl;
        std::cerr << "=== headers ===" << std::endl;

        for (auto&& [header, values] : outgoingRequest.Headers()) {
            std::cerr << "  " << header << ":";

            if (values.size() == 1) {
                std::cerr << " " << values.at(0) << std::endl;

            } else if (values.size() >= 2) {
                for (auto&& value : values) {
                    std::cerr << std::endl << "  " << value << std::endl;
                }

            } else {
                std::cerr << " (empty)" << std::endl;
            }
        }
        std::cerr << "=== end of headers ===" << std::endl;

        std::cerr << "=== parts ===" << std::endl;

        for (auto&& part : outgoingRequest.Parts()) {
            std::cerr << "[part]" << std::endl;
            std::string contentDisposition;
            NHTTP::THeaderParams contentDispositionParams;
            NHTTPUtils::ParseHeader(
                    part.Headers(),
                    "content-disposition",
                    contentDisposition,
                    contentDispositionParams
            );
            std::cerr << "  content-length: " << part.ContentLength() << std::endl;
            std::cerr << "  content-disposition: " << contentDisposition << std::endl;
            std::cerr << "  content-disposition-params: " << std::endl;

            for (auto [key, value]: contentDispositionParams) {
                std::cerr << "    key='" << key << "', value='" << value << "'" << std::endl;
            }

            for (auto [name, value] : part.Headers()) {
                std::cerr << "  [header] " << name << ": " << std::endl;

                for (auto v : value) {
                    std::cerr << "    " << v << std::endl;
                }
            }

            std::cerr << "  [content]" << std::string(part.Content(), part.ContentLength())
                      << "[/content]" << std::endl;
            std::cerr << "[/part]" << std::endl;
        }

        std::cerr << "=== end of parts ===" << std::endl;
        std::cerr << "== END OF OUTGOING REQUEST == " << std::endl;
    }
#endif

    void TRouterDProxyHandler::Iter(std::shared_ptr<TRouterDRequest> request, const std::vector<std::string>& args) const {
        auto&& graph = request->GetGraph();

        while (true) {
            bool somethingHappened(false);
            std::vector<std::string> failedServices;

            // schedule next possible request
            for (auto&& treeIt : graph.Tree) {
                if (!treeIt.second.empty() || request->IsInProgress(treeIt.first)) {
                    // service has unprocessed dependency or is already being processed
                    continue;
                }

                somethingHappened = true; // found service ready to be requested

#ifdef AC_DEBUG_ROUTERD_PROXY
                std::cerr << "graph.Services.at(" << treeIt.first << ");" << std::endl;
#endif

                const auto& service = graph.Services.at(treeIt.first);
                const auto& host = GetHost(service.HostsFrom);

                // try to connect (no sending yet), and schedule response behavior in a callback
                auto rv = request->AwaitHTTP(host.Addr.c_str(), host.Port, [this, request, &service, args](
                    std::shared_ptr<NHTTP::TIncomingResponse> response,
                    std::shared_ptr<NHTTPServer::TClientBase> client
                ) {
                    client->Drop(); // TODO
                    request->NewReply(service.Name);
                    bool serviceReplyProcessed(false);

                    if (response->ContentType() == std::string("multipart/x-ac-routerd")) {

                        for (const auto& part : response->Parts()) {
                            std::string partName;
                            NStringUtils::Strip(part.ContentDispositionParams().at("filename"), partName, 2, "\"'");

                            if (partName == service.Name) {
                                serviceReplyProcessed = true;
                            }

                            ProcessServiceResponse(request, response, partName, &part, /* contentDispositionFormData = */false);
                        }
                    } else {
                        if (!service.SaveAs.empty()) {
                            ProcessServiceResponse(request, response, service.SaveAs, response.get());
                        } else {
                            ProcessServiceResponse(request, response, service.Name, response.get());
                            serviceReplyProcessed = true;
                        }
                    }
                    if (!serviceReplyProcessed) {
                        ServiceReplied(request, service.Name);
                    }

                    Iter(request, args); // recursion depth is limited by graph size, which is small.
                });

                if (!rv) { // could not connect
                    failedServices.push_back(service.Name);
                    continue;
                }

                request->NewRequest(service.Name);

#ifdef AC_DEBUG_ROUTERD_PROXY
                PrintOutgoingRequest(request);
#endif
                // schedule payload to be sent to connected service
                if (!service.SendRawOutputOf.empty()) {
                    auto&& outgoingRequest = request->GetOutGoingRequest();
                    auto matchingPart = outgoingRequest.PartByName(service.SendRawOutputOf);

                    if (matchingPart) {
#ifdef AC_DEBUG_ROUTERD_PROXY
                        std::cerr << "to service " << service.Name
                                  << " will send_raw_output_of " << service.SendRawOutputOf << std::endl;
#endif
                        rv->PushWriteQueueData(matchingPart->GetBody());

                    } else { // should not happen: we are demanding proper dependencies
                        request->Send500();
                        std::cerr << "raw output part not found, issuing 500" << std::endl;
                        break;
                    }

                } else {
                    auto msg = request->OutgoingRequest(service.Path, args);
                    msg.Memorize(request);

                    rv->PushWriteQueueData(msg);
                }
            }

            // erase failed services from graph
            for (const auto& name : failedServices) {
#ifdef AC_DEBUG_ROUTERD_PROXY
                std::cerr << "graph.Tree.erase(" << name << "); // as failed" << std::endl;
#endif
                graph.Tree.erase(name);
            }
#ifdef AC_DEBUG_ROUTERD_PROXY
            std::cerr << "request->InProgressCount() == " << request->InProgressCount() << std::endl;
#endif

            // decide whether to continue loop while(true), exit with 500 (no way to complete request) or exit normally via break.
            if (request->InProgressCount() == 0) { // if we couldn't send any requests
#ifdef AC_DEBUG_ROUTERD_PROXY
               std::cerr << "couldn't send any requests" << std::endl;
#endif
                if (somethingHappened) { // but tried to
#ifdef AC_DEBUG_ROUTERD_PROXY
                    std::cerr << "but tried to" << std::endl;
#endif
                    if (graph.Tree.empty()) { // and there are no services left
                        if (!request->IsResponseSent()) {
                            request->Send500();
#ifdef AC_DEBUG_ROUTERD_PROXY
                            std::cerr << "(1) response was NOT sent, issuing 500" << std::endl;
#endif
                        } else {
#ifdef AC_DEBUG_ROUTERD_PROXY
                            std::cerr << "response was sent" << std::endl;
#endif
                        }

                    } else { // and still have services to try
#ifdef AC_DEBUG_ROUTERD_PROXY
                        std::cerr << "still have services to try" << std::endl;
#endif
                        continue;
                    }

                } else { // and won't send any
#ifdef AC_DEBUG_ROUTERD_PROXY
                    std::cerr << "wont send any" << std::endl;
#endif
                    if (!request->IsResponseSent()) {
#ifdef AC_DEBUG_ROUTERD_PROXY
                        std::cerr << "(2) response was NOT sent, issuing 500" << std::endl;
#endif
                        request->Send500();

                    } else {
#ifdef AC_DEBUG_ROUTERD_PROXY
                        std::cerr << "something was sent, which is ok" << std::endl;
#endif
                    }
                }
            }

            break;
        }
    }

    void TRouterDProxyHandler::ServiceReplied(std::shared_ptr<TRouterDRequest> request, const std::string& serviceName) const {
        auto&& graph = request->GetGraph();
        const auto& it1 = graph.ReverseTree.find(serviceName);

#ifdef AC_DEBUG_ROUTERD_PROXY
        std::cerr << "ServiceReplied:" << serviceName << std::endl;
#endif

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
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - request->StartTime()).count();
                size_t statusCode = response->StatusCode();
                const auto& statusCodeHint = message->HeaderValue("x-ac-routerd-statuscode");

                if (!statusCodeHint.empty()) {
                    NStringUtils::FromString(statusCodeHint, statusCode);
                }

                TStatReport report;
                report.OutputStatusCode = statusCode;
                report.TotalTime = elapsed;
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
