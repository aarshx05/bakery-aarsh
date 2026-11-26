// libFuzzer harness for GroupFrameTransformer H264/VP8 parsing
// Compile: clang++ -std=c++17 -fsanitize=address,undefined,fuzzer \
//   -I<webrtc_src> -I<tgcalls_src> \
//   fuzz_group_frame.cpp <tgcalls_obj_files> <webrtc_libs> \
//   -o fuzz_group_frame
//
// Exercises: calculateH264FramePlaintextHeaderSize and related NAL parsing
// Attack model: malicious group call participant sending crafted video frames.

#include "group/GroupFrameTransformer.h"
#include <cstdint>
#include <cstddef>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    uint8_t selector = data[0];
    data++;
    size--;

    webrtc::ArrayView<const uint8_t> frame(data, size);

    switch (selector & 0x03) {
        case 0: {
            // H264 plaintext header size calculation
            bool isKeyframe = false;
            tgcalls::calculateH264FramePlaintextHeaderSize(frame, &isKeyframe);
            break;
        }
        case 1: {
            // Audio frame decryption stub (needs transform function)
            // In real fuzzing, provide a mock transform that returns the input.
            // This exercises the extension parsing at the end of decrypted audio.
            std::vector<uint8_t> mockDecrypted(data, data + size);
            uint8_t audioLevel = 0;
            bool hasSpeech = false;
            bool hadExtension = false;
            // The actual transform function is needed here - this is a skeleton.
            // In the full harness, mock the GroupEncryptDecryptFunction.
            break;
        }
        case 2: {
            // ValidateEncryptedFrame
            if (size > 4) {
                size_t plaintextHeaderSize = data[0] % size;
                tgcalls::ValidateEncryptedFrame(
                    webrtc::ArrayView<const uint8_t>(data + 1, size - 1),
                    plaintextHeaderSize);
            }
            break;
        }
        case 3: {
            // calculateSliceHeaderBytesForPpsId
            // This uses webrtc::BitstreamReader to parse Exp-Golomb coded values
            tgcalls::calculateSliceHeaderBytesForPpsId(frame);
            break;
        }
    }

    return 0;
}
