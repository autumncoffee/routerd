#pragma once

#include <string>
#include <ac-common/spin_lock.hpp>
#include <unordered_map>
#include <set>

namespace NAC {
    struct TStatReport {
        size_t OutputStatusCode = 0;
        size_t TotalTime = 0;
    };

    struct TTotalTime {
        size_t TotalTime = 0;
        size_t ReportCount = 0;
    };

    struct TStats : public TTotalTime {
        std::unordered_map<size_t, size_t> OutputStatusCodes;
        std::unordered_map<size_t, TTotalTime> TotalTimes;
    };

    class TStatWriter {
    public:
        TStatWriter(const std::set<size_t>& responseTimeBuckets);

        void Write(const TStatReport& report);

        TStats Extract();

    private:
        std::set<size_t> ResponseTimeBuckets;
        NUtils::TSpinLock Lock;
        TStats Stats;
    };
}
