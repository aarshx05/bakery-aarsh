// libFuzzer harness for signaling message parsing
// Compile: clang++ -std=c++17 -fsanitize=address,undefined,fuzzer \
//   -I<webrtc_src> -I<tgcalls_src> \
//   fuzz_signaling_parse.cpp <tgcalls_obj_files> <webrtc_libs> \
//   -o fuzz_signaling_parse
//
// Exercises: signaling::Message::parse (v2) and signaling_4_0_0::Message::parse
// These parse JSON signaling messages from decrypted data.
// Attack model: malicious peer sends crafted JSON signaling.

#include "v2/Signaling.h"
#include "v2_4_0_0/Signaling_4_0_0.h"
#include <cstdint>
#include <cstddef>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    uint8_t selector = data[0];
    data++;
    size--;

    std::vector<uint8_t> input(data, data + size);

    switch (selector & 0x01) {
        case 0: {
            // v2 signaling parse
            tgcalls::signaling::Message::parse(input);
            break;
        }
        case 1: {
            // v2_4_0_0 (legacy) signaling parse
            tgcalls::signaling_4_0_0::Message::parse(input);
            break;
        }
    }

    return 0;
}
