// Fuzz harness for tgcalls Message deserialization
// Tests DeserializeMessage() and DeserializeRawMessage() with ASAN+UBSAN

namespace tgcalls {
enum class VideoState { Inactive, Paused, Active };
enum class AudioState { Muted, Active };
}

#include "drivers/fuzz_driver.h"
#include "Message.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    uint8_t selector = data[0];
    data++;
    size--;

    webrtc::ByteBufferReader reader(webrtc::ArrayView<const uint8_t>(
        const_cast<uint8_t*>(data), size));

    switch (selector & 0x03) {
        case 0:
            tgcalls::DeserializeMessage(reader, true);
            break;
        case 1:
            tgcalls::DeserializeMessage(reader, false);
            break;
        case 2:
            tgcalls::DeserializeRawMessage(reader, true);
            break;
        case 3:
            tgcalls::DeserializeRawMessage(reader, false);
            break;
    }

    // Also test repeated deserialization loop
    if ((selector & 0x04) != 0) {
        webrtc::ByteBufferReader reader2(webrtc::ArrayView<const uint8_t>(
            const_cast<uint8_t*>(data), size));
        for (int i = 0; i < 100 && reader2.Length() > 0; i++) {
            auto msg = tgcalls::DeserializeMessage(reader2, false);
            if (!msg.has_value()) break;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    return fuzz::runDriver(argc, argv, 131072);
}
