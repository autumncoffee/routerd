#include "stat.hpp"
#include <utility>

namespace NAC {
    void TStatWriter::Write(const TStatReport& report) {
        NUtils::TSpinLockGuard guard(Lock);

        if (Stats.OutputStatusCodes.count(report.OutputStatusCode) > 0) {
            ++Stats.OutputStatusCodes[report.OutputStatusCode];

        } else {
            Stats.OutputStatusCodes[report.OutputStatusCode] = 1;
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
