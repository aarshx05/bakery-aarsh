#pragma once
#include <string>
#include <vector>
#include <optional>

namespace webrtc {

struct RtpHeaderExtensionId {
    int value_ = 0;
    RtpHeaderExtensionId() = default;
    explicit RtpHeaderExtensionId(int v) : value_(v) {}
    int value() const { return value_; }
    operator int() const { return value_; }
};

struct RtpExtension {
    std::string uri;
    RtpHeaderExtensionId id;
    bool encrypt = false;

    RtpExtension() = default;
    RtpExtension(const std::string& u, RtpHeaderExtensionId i, bool e = false)
        : uri(u), id(i), encrypt(e) {}
    RtpExtension(const std::string& u, int i, bool e = false)
        : uri(u), id(RtpHeaderExtensionId(i)), encrypt(e) {}

    bool operator==(const RtpExtension& o) const {
        return uri == o.uri && id.value() == o.id.value() && encrypt == o.encrypt;
    }
};

struct RtpHeaderExtensionCapability {
    std::string uri;
    int preferred_id = 0;

    RtpHeaderExtensionCapability() = default;
    RtpHeaderExtensionCapability(const std::string& u, int id)
        : uri(u), preferred_id(id) {}
};

} // namespace webrtc
