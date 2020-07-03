#include "request.hpp"
#include <ac-common/str.hpp>
#include "utils.hpp"
#include <string.h>
#include <pcrecpp.h>

namespace NAC {
    TRouterDRequest::TArgs TRouterDRequest::TArgs::FromConfig(const nlohmann::json& config) {
        TArgs out;

        if (config.count("allow_nested_requests") > 0) {
            out.AllowNestedRequests = config["allow_nested_requests"].get<bool>();
        }

        return out;
    }

    NHTTP::TResponse TRouterDRequest::PreparePart(const std::string& partName) const {
        NHTTP::TResponse out;
        out.Header("Content-Disposition", "form-data; name=\"" + partName + "\"; filename=\"" + partName + "\"");
        out.Header("Content-Type", "application/octet-stream");

        return out;
    }

    void TRouterDRequest::AddPart(NHTTP::TResponse&& part) {
        Out().AddPart(std::move(part));
    }

    NHTTP::TResponse& TRouterDRequest::Out() {
        if (OutgoingRequestInited) {
            return OutgoingRequest_;
        }

        OutgoingRequestInited = true;

        const std::string firstLine(FirstLine());
        const size_t pathStart(Method().size() + 1);
        const size_t pathSize(firstLine.size() - pathStart - (Protocol().size() + 1));

        OutgoingRequest_.FirstLine("POST " + std::string(firstLine.data() + pathStart, pathSize) + " " + Protocol() + "\r\n");

        const bool isNested(Args.AllowNestedRequests && !HeaderValue("x-ac-routerd").empty() && !Parts().empty());

        for (const auto& header : Headers()) {
            if (isNested) {
                if (
                    (header.first == std::string("content-type"))
                    || (header.first == std::string("content-length"))
                ) {
                    continue;
                }

            } else {
                if (strncmp(header.first.data(), "x-ac-routerd", 12) == 0) {
                    continue;
                }

                if (RemapHeader(header, "content-length", "X-AC-RouterD-Content-Length", OutgoingRequest_)) {
                    continue;
                }

                if (RemapHeader(header, "content-type", "X-AC-RouterD-CType", OutgoingRequest_)) {
                    continue;
                }
            }

            AddHeader(header, OutgoingRequest_);
        }

        if (!isNested) {
            OutgoingRequest_.Header("X-AC-RouterD-Method", Method());
            OutgoingRequest_.Header("X-AC-RouterD", DefaultChunkName());
        }

        OutgoingRequest_.Header("Content-Type", "multipart/form-data");

        PrepareOutgoingRequest(OutgoingRequest_);

        if (isNested) {
            for (const auto& in : Parts()) {
                NHTTP::TResponse out;
                CopyHeaders(in.Headers(), out);

                if (in.ContentLength() > 0) {
                    out.Wrap(in.ContentLength(), in.Content());
                }

                AddPart(std::move(out));
            }

        } else {
            auto part = PreparePart(DefaultChunkName());

            if (ContentLength() > 0) {
                part.Wrap(ContentLength(), Content());
            }

            AddPart(std::move(part));
        }

        return OutgoingRequest_;
    }

    TBlobSequence TRouterDRequest::OutgoingRequest(const std::string& path_, const std::vector<std::string>& args) {
        if (path_.empty()) {
            return (TBlobSequence)Out();
        }

        std::string path(path_);

        for (size_t i = 0; i < args.size(); ++i) {
            pcrecpp::RE(
                std::string("{\\s*")
                + std::to_string(i + 1)
                + std::string("\\s*}")

            ).GlobalReplace(args.at(i), &path);
        }

        const auto& base = Out();
        NHTTP::TResponse out;

        {
            const auto& firstLineParts = NStringUtils::Split(base.FirstLine(), ' ');

            if (firstLineParts.size() > 1) {
                const auto& pathParts = NStringUtils::Split(firstLineParts.at(1), '?');
                std::string firstLine((std::string)firstLineParts.at(0) + " " + path);

                for (size_t i = 1; i < pathParts.size(); ++i) {
                    firstLine += "?" + (std::string)pathParts.at(i);
                }

                for (size_t i = 2; i < firstLineParts.size(); ++i) {
                    firstLine += " " + (std::string)firstLineParts.at(i);
                }

                out.FirstLine(std::move(firstLine));

            } else {
                out.FirstLine(base.FirstLine());
            }
        }

        for (const auto& baseHeader : base.Headers()) {
            for (const auto& value : baseHeader.second) {
                out.Header(baseHeader.first, value);
            }
        }

        for (const auto& basePart : base.Parts()) {
            NHTTP::TResponse part;

            if (basePart.ContentLength() > 0) {
                part.Wrap(basePart.ContentLength(), basePart.Content());
            }

            for (const auto& baseHeader : basePart.Headers()) {
                for (const auto& value : baseHeader.second) {
                    part.Header(baseHeader.first, value);
                }
            }

            out.AddPart(std::move(part));
        }

        return (TBlobSequence)out;
    }
}
