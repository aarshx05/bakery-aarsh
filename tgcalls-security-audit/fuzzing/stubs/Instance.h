#pragma once
#include <cstdint>
#include <memory>
#include <array>
#include <functional>
#include <vector>
#include <string>
#include <map>

namespace tgcalls {

struct EncryptionKey {
    static constexpr int kSize = 256;
    std::shared_ptr<const std::array<uint8_t, kSize>> value;
    bool isOutgoing = false;

    EncryptionKey() = default;
    EncryptionKey(
        std::shared_ptr<const std::array<uint8_t, kSize>> const value,
        bool isOutgoing)
    : value(value), isOutgoing(isOutgoing) {}
};

enum class VideoState { Inactive, Paused, Active };
enum class AudioState { Muted, Active };

} // namespace tgcalls
