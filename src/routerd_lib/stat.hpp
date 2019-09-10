#pragma once

#include <string>
#include <ac-common/spin_lock.hpp>
#include <unordered_map>

namespace NAC {
    struct TStatReport {
        size_t OutputStatusCode = 0;
        size_t TotalTime = 0;
    };

    struct TStats {
        std::unordered_map<size_t, size_t> OutputStatusCodes;
        size_t TotalTime = 0;
        size_t ReportCount = 0;
    };

    class TStatWriter {
    public:
        void Write(const TStatReport& report);

        TStats Extract();

    private:
        NUtils::TSpinLock Lock;
        TStats Stats;
    };
}
