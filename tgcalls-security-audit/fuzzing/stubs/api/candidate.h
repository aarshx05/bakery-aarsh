#pragma once
#include <string>

namespace webrtc {

class Candidate {
public:
    Candidate() = default;

    const std::string& id() const { return id_; }
    void set_id(const std::string& id) { id_ = id; }

    const std::string& component() const { return component_; }
    const std::string& protocol() const { return protocol_; }
    const std::string& address() const { return address_; }
    uint32_t priority() const { return priority_; }
    const std::string& type() const { return type_; }
    const std::string& network_name() const { return network_name_; }
    uint32_t generation() const { return generation_; }
    const std::string& foundation() const { return foundation_; }
    const std::string& related_address() const { return related_address_; }
    const std::string& tcptype() const { return tcptype_; }

    std::string ToString() const { return sdp_; }
    static Candidate FromSdp(const std::string& sdp) {
        Candidate c;
        c.sdp_ = sdp;
        return c;
    }

private:
    std::string id_;
    std::string component_;
    std::string protocol_;
    std::string address_;
    uint32_t priority_ = 0;
    std::string type_;
    std::string network_name_;
    uint32_t generation_ = 0;
    std::string foundation_;
    std::string related_address_;
    std::string tcptype_;
    std::string sdp_;
};

} // namespace webrtc
