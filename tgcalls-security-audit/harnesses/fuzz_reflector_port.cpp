// libFuzzer harness for ReflectorPort packet handling
// Compile: clang++ -std=c++17 -fsanitize=address,undefined,fuzzer \
//   -I<webrtc_src> -I<tgcalls_src> \
//   fuzz_reflector_port.cpp <tgcalls_obj_files> <webrtc_libs> \
//   -o fuzz_reflector_port
//
// Exercises: ReflectorPort::HandleIncomingPacket
// Attack model: network attacker sending crafted reflector packets.
// This is PRE-DECRYPTION, so no key is needed.

#include "v2/ReflectorPort.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

// Standalone parseHex fuzzer (the function is static in ReflectorPort.cpp,
// so either expose it via a test header or duplicate the logic here).
static std::vector<uint8_t> parseHex(std::string const &hex) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        uint8_t high = 0, low = 0;
        char c = hex[i];
        if (c >= '0' && c <= '9') high = c - '0';
        else if (c >= 'a' && c <= 'f') high = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') high = c - 'A' + 10;
        c = hex[i + 1];
        if (c >= '0' && c <= '9') low = c - '0';
        else if (c >= 'a' && c <= 'f') low = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') low = c - 'A' + 10;
        result.push_back((high << 4) | low);
    }
    return result;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    uint8_t selector = data[0];
    data++;
    size--;

    switch (selector & 0x01) {
        case 0: {
            // parseHex with fuzzer-generated string
            std::string hexStr(reinterpret_cast<const char *>(data), size);
            auto result = parseHex(hexStr);
            volatile size_t s = result.size();
            (void)s;
            break;
        }
        case 1: {
            // ReflectorPort::HandleIncomingPacket requires a full ReflectorPort
            // object. This is a skeleton that needs the WebRTC build system.
            // In the full harness, construct a ReflectorPort with test config
            // and feed packets to HandleIncomingPacket.
            //
            // The key parsing logic to exercise:
            //   - peer_tag validation (first 12 of 16 bytes)
            //   - special packet detection (12 bytes of 0xFF at offset 16)
            //   - data packet parsing (senderTag + dataSize at offset 16)
            //   - dataSize bounds check: dataSize > size - 16 - 4 - 4
            break;
        }
    }

    return 0;
}
