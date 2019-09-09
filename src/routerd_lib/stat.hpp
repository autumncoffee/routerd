#pragma once

#include <string>
#include <ac-common/spin_lock.hpp>
#include <unordered_map>

namespace NAC {
    struct TStatReport {
        size_t OutputStatusCode = 0;
    };

    struct TStats {
        std::unordered_map<size_t, size_t> OutputStatusCodes;
    };

    class TStatWriter {
    public:
        TStatWriter() = delete;
        TStatWriter(const std::string& graphName);

        void Write(const TStatReport& report);

        const std::string& GraphName() const {
            return GraphName_;
        }

        TStats Extract();

    private:
        std::string GraphName_;
        NUtils::TSpinLock Lock;
        TStats Stats;
    };
}
