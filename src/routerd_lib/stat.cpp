#include "stat.hpp"
#include <utility>

namespace NAC {
    TStatWriter::TStatWriter(const std::set<size_t>& responseTimeBuckets)
        : ResponseTimeBuckets(responseTimeBuckets)
    {
    }

    void TStatWriter::Write(const TStatReport& report) {
        size_t totalTimeBucket(0);

        for (size_t bucket : ResponseTimeBuckets) {
            if (report.TotalTime < bucket) {
                break;
            }

            totalTimeBucket = bucket;
        }

        NUtils::TSpinLockGuard guard(Lock);

        ++Stats.ReportCount;

        if (Stats.OutputStatusCodes.count(report.OutputStatusCode) > 0) {
            ++Stats.OutputStatusCodes[report.OutputStatusCode];

        } else {
            Stats.OutputStatusCodes[report.OutputStatusCode] = 1;
        }

        Stats.TotalTime += report.TotalTime;

        auto totalTimeIt = Stats.TotalTimes.find(totalTimeBucket);

        if (totalTimeIt == Stats.TotalTimes.end()) {
            Stats.TotalTimes[totalTimeBucket] = TTotalTime{report.TotalTime, 1};

        } else {
            totalTimeIt->second.TotalTime += report.TotalTime;
            ++totalTimeIt->second.ReportCount;
        }
    }

    TStats TStatWriter::Extract() {
        TStats out;

        {
            NUtils::TSpinLockGuard guard(Lock);
            std::swap(out, Stats);
        }

        return out;
    }
}
