#include "proxy.hpp"
#include <stdlib.h>
#include <ac-common/str.hpp>
#include <utility>
#include <random>
#include <routerd_lib/utils.hpp>
#include <ac-library/http/server/await_client.hpp>

namespace NAC {
    void TRouterDProxyHandler::Handle(
        std::shared_ptr<TRouterDRequest> request,
        const std::vector<std::string>& /* args */
    ) {
        Iter(request);
    }

    const TServiceHost& TRouterDProxyHandler::GetHost(const std::string& service) const {
        const auto& hosts = Hosts.at(service);

        if (hosts.size() > 1) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::uniform_int_distribution<size_t> dis(0, hosts.size() - 1);

            return hosts.at(dis(g));
        }

        return hosts.front();
    }

    void TRouterDProxyHandler::Iter(std::shared_ptr<TRouterDRequest> request) const {
        auto msg = request->OutgoingRequest();
        msg.Memorize(request);
        bool requestSent(false);

        for (const auto& service : Order.at(request->GetStage())) {
            const auto& host = GetHost(service);
            auto rv = request->AwaitHTTP(host.Addr.c_str(), host.Port, [this, request, service](
                std::shared_ptr<NHTTP::TIncomingResponse> response,
                std::shared_ptr<NHTTPServer::TClientBase> client
            ) {
                client->Drop(); // TODO
                request->NewReply();

                if (service == std::string("output")) { // TODO
                    NHTTP::TResponse out;
                    out.FirstLine(response->FirstLine() + "\r\n");
                    CopyHeaders(response->Headers(), out);
                    out.Wrap(response->ContentLength(), (char*)response->Content());
                    out.Memorize(response);

                    request->Send(out);
                }

                auto s = TBlobSequence::Construct(response->ContentLength(), response->Content());
                s.Memorize(response);
                request->PushData(service, std::move(s));

                if (request->ReplyCount() >= Order.at(request->GetStage()).size()) {
                    OnStageDone(request);
                }
            });

            if (!rv) {
                request->NewReply();
                continue;
            }

            rv->PushWriteQueueData(msg);
            requestSent = true;
        }

        if (!requestSent) {
            OnStageDone(request);
        }
    }

    void TRouterDProxyHandler::OnStageDone(std::shared_ptr<TRouterDRequest> request) const {
        request->NextStage();

        if (request->GetStage() < Order.size()) {
            Iter(request);
            return;
        }

        if (!request->IsResponseSent()) {
            request->Send500();
            return;
        }
    }
}
