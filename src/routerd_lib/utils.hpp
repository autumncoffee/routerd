#pragma once

#include <ac-library/http/response.hpp>
#include <ac-library/http/utils/headers.hpp>

namespace NAC {
    static inline void AddHeader(
        const std::string& name,
        const NHTTPLikeParser::THeaders::mapped_type& values,
        NHTTP::TResponse& dst,
        bool filterOutContentDispositionFormData = false
    ) {
        for (const auto& value : values) {
            if (filterOutContentDispositionFormData) {
                std::string value_;
                NHTTP::THeaderParams params;

                NHTTPUtils::ParseHeaderValue(value, value_, params);

                if (value_ == std::string("form-data")) {
                    continue;
                }
            }

            dst.Header(name, value);
        }
    }

    static inline void AddHeader(
        const NHTTPLikeParser::THeaders::value_type& header,
        NHTTP::TResponse& dst,
        bool filterOutContentDispositionFormData = false
    ) {
        AddHeader(header.first, header.second, dst, filterOutContentDispositionFormData);
    }

    static inline void CopyHeaders(
        const NHTTPLikeParser::THeaders& headers,
        NHTTP::TResponse& to,
        const bool contentType = true,
        const bool contentDispositionFormData = true
    ) {
        for (const auto& header : headers) {
            if (header.first == "content-length") {
                continue;
            }

            if (!contentType && (header.first == "content-type")) {
                continue;
            }

            bool filterOutContentDispositionFormData(false);

            if (!contentDispositionFormData && (header.first == "content-disposition")) {
                filterOutContentDispositionFormData = true;
            }

            AddHeader(header, to, filterOutContentDispositionFormData);
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
