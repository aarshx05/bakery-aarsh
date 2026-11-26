#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "api/video_codecs/sdp_video_format.h"

namespace webrtc {

enum class MediaType { AUDIO, VIDEO, DATA, UNSUPPORTED };
enum class MediaProtocolType { kRtp, kSctp };

struct FeedbackParam {
    std::string id_, param_;
    FeedbackParam() = default;
    FeedbackParam(const std::string& i, const std::string& p) : id_(i), param_(p) {}
    const std::string& id() const { return id_; }
    const std::string& param() const { return param_; }
};

struct FeedbackParams {
    std::vector<FeedbackParam> params_;
    const std::vector<FeedbackParam>& params() const { return params_; }
    void Add(const FeedbackParam& p) { params_.push_back(p); }
};

struct Codec {
    int id = 0;
    std::string name;
    int clockrate = 0;
    int channels = 0;
    std::map<std::string, std::string> params;
    FeedbackParams feedback_params;

    void AddFeedbackParam(const FeedbackParam& p) { feedback_params.Add(p); }
    void SetParam(const std::string& key, const std::string& value) { params[key] = value; }
    bool GetParam(const std::string& key, std::string* value) const {
        auto it = params.find(key);
        if (it != params.end()) { *value = it->second; return true; }
        return false;
    }
};

class MediaContentDescription {
public:
    virtual ~MediaContentDescription() = default;
    virtual MediaType type() const = 0;
    virtual const MediaContentDescription* as_audio() const { return this; }
    virtual const MediaContentDescription* as_video() const { return this; }
    std::vector<Codec> codecs_;
    const std::vector<Codec>& codecs() const { return codecs_; }
    void AddCodec(const Codec& c) { codecs_.push_back(c); }
};

class AudioContentDescription : public MediaContentDescription {
public:
    MediaType type() const override { return MediaType::AUDIO; }
};

class VideoContentDescription : public MediaContentDescription {
public:
    MediaType type() const override { return MediaType::VIDEO; }
};

struct ContentInfo {
    MediaProtocolType protocol_ = MediaProtocolType::kRtp;
    std::string name;
    std::shared_ptr<MediaContentDescription> description_;
    ContentInfo() = default;
    ContentInfo(MediaProtocolType p, const std::string& n, std::unique_ptr<MediaContentDescription> d)
        : protocol_(p), name(n), description_(std::move(d)) {}
    const MediaContentDescription* media_description() const { return description_.get(); }
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
