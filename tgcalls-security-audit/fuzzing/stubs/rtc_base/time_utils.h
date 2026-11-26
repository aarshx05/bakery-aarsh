#pragma once
#include <cstdint>
#include <chrono>
#include <arpa/inet.h>

namespace webrtc {

inline int64_t TimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline uint32_t HostToNetwork32(uint32_t v) { return htonl(v); }
inline uint32_t NetworkToHost32(uint32_t v) { return ntohl(v); }

} // namespace webrtc
