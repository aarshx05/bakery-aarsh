#pragma once
#include <string>
#include <vector>
#include <map>
#include "api/video_codecs/sdp_video_format.h"
#include "media/base/media_engine.h"

namespace cricket {

struct ContentInfo {
    std::string name;
    std::string type;
    std::vector<webrtc::Codec> codecs;
};

struct ContentGroup {
    std::string semantics;
    std::vector<std::string> content_names;

    explicit ContentGroup(const std::string& s) : semantics(s) {}

    void AddContentName(const std::string& name) {
        content_names.push_back(name);
    }
    bool HasContentName(const std::string& name) const {
        for (auto& n : content_names) if (n == name) return true;
        return false;
    }
};

} // namespace cricket

namespace webrtc {

using ContentInfo = cricket::ContentInfo;

enum class RtpTransceiverDirection {
    kSendRecv,
    kSendOnly,
    kRecvOnly,
    kInactive,
    kStopped,
};

} // namespace webrtc
