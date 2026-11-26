// libFuzzer harness for EncryptedConnection packet handling
// Compile: clang++ -std=c++17 -fsanitize=address,undefined,fuzzer \
//   -I<webrtc_src> -I<tgcalls_src> \
//   fuzz_encrypted_connection.cpp <tgcalls_obj_files> <webrtc_libs> \
//   -lssl -lcrypto -o fuzz_encrypted_connection
//
// This harness exercises the full decrypt+parse path:
//   handleIncomingPacket -> decrypt -> processPacket -> DeserializeMessage
//   handleIncomingRawPacket -> decrypt -> processRawPacket -> DeserializeRawMessage

#include "EncryptedConnection.h"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace {

// Fixed test key for deterministic fuzzing.
// In a real call, this is derived from DH key exchange.
constexpr uint8_t kTestKeyData[256] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    // ... rest initialized to 0 by default
};

class FuzzEncryptedConnectionCallback {
public:
    void onDecryptedMessage(int type, const webrtc::CopyOnWriteBuffer &data) {
        // Consume to prevent optimization
        volatile size_t s = data.size();
        (void)s;
    }
};

struct FuzzState {
    tgcalls::EncryptionKey key;
    std::unique_ptr<tgcalls::EncryptedConnection> conn_transport;
    std::unique_ptr<tgcalls::EncryptedConnection> conn_signaling;

    FuzzState() {
        auto keyValue = std::make_shared<std::vector<uint8_t>>(
            kTestKeyData, kTestKeyData + sizeof(kTestKeyData));
        key.value = keyValue;
        key.isOutgoing = false; // receiver side

        conn_transport = std::make_unique<tgcalls::EncryptedConnection>(
            tgcalls::EncryptedConnection::Type::Transport,
            key,
            [](int, int) {});

        conn_signaling = std::make_unique<tgcalls::EncryptedConnection>(
            tgcalls::EncryptedConnection::Type::Signaling,
            key,
            [](int, int) {});
    }
};

FuzzState *getState() {
    static FuzzState state;
    return &state;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    auto *state = getState();

    // First byte selects the target function
    uint8_t selector = data[0];
    data++;
    size--;

    switch (selector & 0x03) {
        case 0: {
            // handleIncomingPacket (transport encryption, full packet path)
            state->conn_transport->handleIncomingPacket(
                reinterpret_cast<const char *>(data), size);
            break;
        }
        case 1: {
            // handleIncomingRawPacket (transport encryption, raw packet path)
            state->conn_transport->handleIncomingRawPacket(
                reinterpret_cast<const char *>(data), size);
            break;
        }
        case 2: {
            // decryptRawPacket (signaling encryption)
            webrtc::CopyOnWriteBuffer buf(
                std::span<const uint8_t>(data, size));
            state->conn_signaling->decryptRawPacket(buf);
            break;
        }
        case 3: {
            // decryptFullPlaintextPacket (signaling encryption, WASM path)
            webrtc::CopyOnWriteBuffer buf(
                std::span<const uint8_t>(data, size));
            state->conn_signaling->decryptFullPlaintextPacket(buf);
            break;
        }
    }

    return 0;
}
