// Harness for GroupFrameTransformer H264/VP8 parsing under ASAN+UBSAN
#include "group/GroupFrameTransformer.h"
#include <cstdint>
#include <cstddef>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    uint8_t selector = data[0];
    data++;
    size--;

    switch (selector & 0x07) {
        case 0: {
            // H264 plaintext header size calculation
            webrtc::ArrayView<const uint8_t> frame(
                const_cast<uint8_t*>(data), size);
            uint32_t headerSize = 0;
            tgcalls::calculateH264FramePlaintextHeaderSize(frame, headerSize);
            break;
        }
        case 1: {
            // VP8 plaintext header size calculation
            webrtc::ArrayView<const uint8_t> frame(
                const_cast<uint8_t*>(data), size);
            uint32_t headerSize = 0;
            tgcalls::calculateVp8FramePlaintextHeaderSize(frame, headerSize);
            break;
        }
        case 2: {
            // ValidateEncryptedFrame with H264 type
            if (size > 1) {
                uint8_t prefix = data[0] % (size > 1 ? size - 1 : 1);
                std::vector<uint8_t> frameCopy(data + 1, data + size);
                webrtc::ArrayView<uint8_t> frame(frameCopy.data(), frameCopy.size());
                tgcalls::ValidateEncryptedFrame(
                    tgcalls::FrameTransformerPayloadType::H264,
                    frame, prefix);
            }
            break;
        }
        case 3: {
            // ValidateEncryptedFrame with VP8 type
            if (size > 1) {
                uint8_t prefix = data[0] % (size > 1 ? size - 1 : 1);
                std::vector<uint8_t> frameCopy(data + 1, data + size);
                webrtc::ArrayView<uint8_t> frame(frameCopy.data(), frameCopy.size());
                tgcalls::ValidateEncryptedFrame(
                    tgcalls::FrameTransformerPayloadType::VP8,
                    frame, prefix);
            }
            break;
        }
        case 4: {
            // H264 frame with NAL start codes: 00 00 00 01 + NAL header
            // Build a frame with attacker-controlled NAL content
            std::vector<uint8_t> frame;
            frame.push_back(0); frame.push_back(0); frame.push_back(0); frame.push_back(1);
            frame.insert(frame.end(), data, data + size);
            webrtc::ArrayView<const uint8_t> view(frame.data(), frame.size());
            uint32_t headerSize = 0;
            tgcalls::calculateH264FramePlaintextHeaderSize(view, headerSize);
            break;
        }
        case 5: {
            // Audio frame encrypt/decrypt round-trip with mock transform
            tgcalls::GroupEncryptDecryptFunction mockTransform =
                [](std::vector<uint8_t> const& d, int64_t, bool isEncrypt, int32_t prefix)
                    -> std::vector<uint8_t> {
                return d; // identity transform
            };
            webrtc::ArrayView<const uint8_t> frame(
                const_cast<uint8_t*>(data), size);
            auto encrypted = tgcalls::encryptGroupAudioFrame(
                mockTransform, 1, frame, 50, true);
            if (!encrypted.empty()) {
                uint8_t level = 0;
                bool speech = false, hadExt = false;
                webrtc::ArrayView<const uint8_t> encView(encrypted.data(), encrypted.size());
                tgcalls::decryptGroupAudioFrame(
                    mockTransform, 1, encView, &level, &speech, &hadExt);
            }
            break;
        }
        case 6: {
            // Audio frame decryption with attacker-controlled data
            tgcalls::GroupEncryptDecryptFunction mockTransform =
                [](std::vector<uint8_t> const& d, int64_t, bool, int32_t)
                    -> std::vector<uint8_t> {
                return d;
            };
            webrtc::ArrayView<const uint8_t> frame(
                const_cast<uint8_t*>(data), size);
            uint8_t level = 0;
            bool speech = false, hadExt = false;
            tgcalls::decryptGroupAudioFrame(
                mockTransform, 1, frame, &level, &speech, &hadExt);
            break;
        }
        case 7: {
            // Video frame encrypt/decrypt round-trip
            tgcalls::GroupEncryptDecryptFunction mockTransform =
                [](std::vector<uint8_t> const& d, int64_t, bool, int32_t)
                    -> std::vector<uint8_t> {
                return d;
            };
            webrtc::ArrayView<const uint8_t> frame(
                const_cast<uint8_t*>(data), size);
            tgcalls::encryptGroupVideoFrame(
                mockTransform, 1, tgcalls::FrameTransformerPayloadType::H264, frame);
            tgcalls::decryptGroupVideoFrame(mockTransform, 1, frame);
            break;
        }
    }

    return 0;
}

#include "driver.h"
