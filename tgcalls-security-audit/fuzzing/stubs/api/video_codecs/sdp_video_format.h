#pragma once
#include <string>
#include <map>

namespace webrtc {

struct SdpVideoFormat {
    std::string name;
    std::map<std::string, std::string> parameters;

    SdpVideoFormat() = default;
    explicit SdpVideoFormat(const std::string& n) : name(n) {}
    SdpVideoFormat(const std::string& n, const std::map<std::string, std::string>& p)
        : name(n), parameters(p) {}

    bool operator==(const SdpVideoFormat& o) const {
        return name == o.name && parameters == o.parameters;
    }
};

} // namespace webrtc
