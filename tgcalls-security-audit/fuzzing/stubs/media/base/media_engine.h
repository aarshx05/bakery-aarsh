#pragma once
#include <string>
#include <map>
#include "api/video_codecs/sdp_video_format.h"

namespace webrtc {

struct Codec {
    int id = 0;
    std::string name;
    int clockrate = 0;
    int channels = 0;
    std::map<std::string, std::string> params;

    void SetParam(const std::string& key, const std::string& value) {
        params[key] = value;
    }

    bool GetParam(const std::string& key, std::string* value) const {
        auto it = params.find(key);
        if (it != params.end()) { *value = it->second; return true; }
        return false;
    }
};

inline Codec CreateAudioCodec(int id, const std::string& name, int clockrate, int channels) {
    Codec c;
    c.id = id;
    c.name = name;
    c.clockrate = clockrate;
    c.channels = channels;
    return c;
}

inline Codec CreateVideoCodec(const SdpVideoFormat& fmt) {
    Codec c;
    c.name = fmt.name;
    c.params = fmt.parameters;
    return c;
}

} // namespace webrtc
