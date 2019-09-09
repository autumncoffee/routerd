#include "stat.hpp"
#include <json.hh>

namespace NAC {
    void TRouterDStatHandler::Handle(
        const std::shared_ptr<NHTTP::TRequest> request,
        const std::vector<std::string>& args
    ) {
        auto out = nlohmann::json::object();

        for (auto&& statWriter : Stats) {
            auto&& stats = statWriter.second->Extract();
            auto&& graphOut = out[statWriter.first] = nlohmann::json::object();
            auto&& outputStatusCodes = graphOut["output_status_codes"] = nlohmann::json::object();

            for (const auto& it : stats.OutputStatusCodes) {
                outputStatusCodes[std::to_string(it.first)] = it.second;
            }
        }

        auto&& response = request->Respond200();
        response.Header("Content-Type", "application/json");
        response.Write(out.dump());

        request->Send(std::move(response));
    }
}
