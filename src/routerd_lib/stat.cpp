#include "stat.hpp"
#include <utility>

namespace NAC {
    void TStatWriter::Write(const TStatReport& report) {
        NUtils::TSpinLockGuard guard(Lock);

        ++Stats.ReportCount;

        if (Stats.OutputStatusCodes.count(report.OutputStatusCode) > 0) {
            ++Stats.OutputStatusCodes[report.OutputStatusCode];

        } else {
            Stats.OutputStatusCodes[report.OutputStatusCode] = 1;
        }

        Stats.TotalTime += report.TotalTime;
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
