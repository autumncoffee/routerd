#pragma once

#include <ac-library/http/response.hpp>

namespace NAC {
    static inline void AddHeader(
        const std::string& name,
        const NHTTPLikeParser::THeaders::mapped_type& values,
        NHTTP::TResponse& dst
    ) {
        for (const auto& value : values) {
            dst.Header(name, value);
        }
    }

    static inline void AddHeader(
        const NHTTPLikeParser::THeaders::value_type& header,
        NHTTP::TResponse& dst
    ) {
        AddHeader(header.first, header.second, dst);
    }

    static inline void CopyHeaders(
        const NHTTPLikeParser::THeaders& headers,
        NHTTP::TResponse& to,
        const bool contentType = true
    ) {
        for (const auto& header : headers) {
            if (header.first == "content-length") {
                continue;
            }

            if (!contentType && (header.first == "content-type")) {
                continue;
            }

            AddHeader(header, to);
        }
    }

    static inline bool RemapHeader(
        const NHTTPLikeParser::THeaders::value_type& header,
        const std::string& from,
        const std::string& to,
        NHTTP::TResponse& dst
    ) {
        if (header.first == from) {
            AddHeader(to, header.second, dst);
            return true;
        }

        return false;
    }
}
