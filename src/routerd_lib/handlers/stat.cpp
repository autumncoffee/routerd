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

            {
                auto&& outputStatusCodes = graphOut["output_status_codes"] = nlohmann::json::object();

                for (const auto& it : stats.OutputStatusCodes) {
                    outputStatusCodes[std::to_string(it.first)] = it.second;
                }
            }

            if (stats.ReportCount > 0) {
                graphOut["avg_time"] = (size_t)(((double)stats.TotalTime / (double)stats.ReportCount) + 0.5);

            } else {
                graphOut["avg_time"] = 0;
            }
        }

        auto&& response = request->Respond200();
        response.Header("Content-Type", "application/json");
        response.Write(out.dump());

        request->Send(std::move(response));
    }
}
