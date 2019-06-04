#include "request.hpp"
#include <ac-common/str.hpp>
#include "utils.hpp"

namespace NAC {
    void TRouterDRequest::PushPartPreamble(const std::string& partName, size_t size) {
        TBlob preamble;
        preamble.Append(std::string(DataInited ? "\r\n" : "") + "--" + Boundary() + "\r\n");
        preamble.Append("Content-Disposition: form-data; name=\"" + partName + "\"; filename=\"" + partName + "\"\r\n");
        preamble.Append("Content-Type: application/octet-stream\r\n");
        preamble.Append("Content-Length: " + std::to_string(size) + "\r\n\r\n");

        Data_.Concat(std::move(preamble));
    }

    TBlobSequence& TRouterDRequest::Data() {
        if (DataInited) {
            return Data_;
        }

        PushPartPreamble(DefaultChunkName(), ContentLength());

        DataInited = true;

        if (ContentLength() > 0) {
            Data_.Concat(ContentLength(), Content());
        }

        return Data_;
    }

    void TRouterDRequest::PushDataImpl(const std::string& partName, TBlobSequence&& input) {
        auto&& data = Data();
        size_t contentLength = 0;

        for (const auto& node : input) {
            contentLength += node.Len;
        }

        PushPartPreamble(partName, contentLength);
        data.Concat(std::move(input));
    }

    TBlobSequence TRouterDRequest::OutgoingRequest() {
        if (!OutgoingRequestInited) {
            OutgoingRequestInited = true;

            const std::string firstLine(FirstLine());
            const size_t pathStart(Method().size() + 1);
            const size_t pathSize(firstLine.size() - pathStart - (Protocol().size() + 1));

            OutgoingRequest_.FirstLine("POST " + std::string(firstLine.data() + pathStart, pathSize) + " " + Protocol() + "\r\n");

            for (const auto& header : Headers()) {
                if (RemapHeader(header, "content-length", "X-AC-RouterD-Content-Length", OutgoingRequest_)) {
                    continue;
                }

                if (RemapHeader(header, "content-type", "X-AC-RouterD-CType", OutgoingRequest_)) {
                    continue;
                }

                AddHeader(header, OutgoingRequest_);
            }

            OutgoingRequest_.Header("X-AC-RouterD-Method", Method());
            OutgoingRequest_.Header("X-AC-RouterD", DefaultChunkName());
            OutgoingRequest_.Header("Content-Type", std::string("multipart/form-data; boundary=") + Boundary());

            PrepareOutgoingRequest(OutgoingRequest_);
        }

        TBlobSequence out;
        TBlob end;
        end << "\r\n--" << Boundary() << "--\r\n";

        {
            auto preamble = OutgoingRequest_.Preamble();
            size_t contentLength = end.Size();

            for (const auto& node : Data()) {
                contentLength += node.Len;
            }

            preamble
                << "Content-Length: "
                << std::to_string(contentLength)
                << "\r\n\r\n"
            ;

            out.Concat(std::move(preamble));
        }

        out.Concat(Data());
        out.Concat(std::move(end));

        return out;
    }
}
