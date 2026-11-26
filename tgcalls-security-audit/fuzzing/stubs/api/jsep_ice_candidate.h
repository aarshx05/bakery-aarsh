#pragma once
#include <string>
#include <memory>
#include "api/candidate.h"

namespace webrtc {

class IceCandidate {
public:
    IceCandidate(const std::string& sdp_mid, int sdp_mline_index, const Candidate& c)
        : sdp_mid_(sdp_mid), sdp_mline_index_(sdp_mline_index), candidate_(c) {}

    static std::unique_ptr<IceCandidate> Create(
        const std::string& sdp_mid, int sdp_mline_index,
        const std::string& sdp) {
        auto c = std::make_unique<IceCandidate>(sdp_mid, sdp_mline_index, Candidate());
        c->sdp_ = sdp;
        return c;
    }

    const Candidate& candidate() const { return candidate_; }
    const std::string& sdp_mid() const { return sdp_mid_; }
    int sdp_mline_index() const { return sdp_mline_index_; }

    std::string ToString() const { return sdp_; }

private:
    std::string sdp_mid_;
    int sdp_mline_index_;
    std::string sdp_;
    Candidate candidate_;
};

// Alias used in tgcalls
using JsepIceCandidate = IceCandidate;

} // namespace webrtc
