// libFuzzer harness for Message deserialization
// Compile: clang++ -std=c++17 -fsanitize=address,undefined,fuzzer \
//   -I<webrtc_src> -I<tgcalls_src> \
//   fuzz_message_deserialize.cpp <tgcalls_obj_files> <webrtc_libs> \
//   -o fuzz_message_deserialize
//
// Exercises: DeserializeMessage and DeserializeRawMessage
// These parse binary TLV data from decrypted packets.
// Attack model: malicious peer provides crafted plaintext.

#include "Message.h"
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    uint8_t selector = data[0];
    data++;
    size--;

    webrtc::ByteBufferReader reader(webrtc::ArrayView<const uint8_t>(data, size));

    switch (selector & 0x03) {
        case 0: {
            // DeserializeMessage, single-message packet
            tgcalls::DeserializeMessage(reader, true);
            break;
        }
        case 1: {
            // DeserializeMessage, multi-message packet
            tgcalls::DeserializeMessage(reader, false);
            break;
        }
        case 2: {
            // DeserializeRawMessage, single-message packet
            tgcalls::DeserializeRawMessage(reader, true);
            break;
        }
        case 3: {
            // DeserializeRawMessage, multi-message packet
            tgcalls::DeserializeRawMessage(reader, false);
            break;
        }
    }

    return 0;
}
