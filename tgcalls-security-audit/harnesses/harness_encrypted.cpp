// Harness for EncryptedConnection packet handling under ASAN+UBSAN
// Exercises: handleIncomingPacket, handleIncomingRawPacket,
// decryptRawPacket, decryptFullPlaintextPacket
#include "EncryptedConnection.h"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace {

constexpr uint8_t kTestKeyData[256] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    // rest zero-initialized
};

static std::shared_ptr<const std::array<uint8_t, 256>> makeTestKey() {
    auto k = std::make_shared<std::array<uint8_t, 256>>();
    std::copy(kTestKeyData, kTestKeyData + 256, k->begin());
    return k;
}

struct FuzzState {
    tgcalls::EncryptionKey key;
    std::unique_ptr<tgcalls::EncryptedConnection> conn_transport;
    std::unique_ptr<tgcalls::EncryptedConnection> conn_signaling;

    FuzzState() : key(makeTestKey(), false) {

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
    uint8_t selector = data[0];
    data++;
    size--;

    switch (selector & 0x07) {
        case 0: {
            // handleIncomingPacket (transport)
            state->conn_transport->handleIncomingPacket(
                reinterpret_cast<const char *>(data), size);
            break;
        }
        case 1: {
            // handleIncomingRawPacket (transport)
            state->conn_transport->handleIncomingRawPacket(
                reinterpret_cast<const char *>(data), size);
            break;
        }
        case 2: {
            // decryptRawPacket (signaling)
            webrtc::CopyOnWriteBuffer buf(data, size);
            state->conn_signaling->decryptRawPacket(buf);
            break;
        }
        case 3: {
            // decryptFullPlaintextPacket (signaling, WASM path)
            webrtc::CopyOnWriteBuffer buf(data, size);
            state->conn_signaling->decryptFullPlaintextPacket(buf);
            break;
        }
        case 4: {
            // handleIncomingPacket (signaling)
            state->conn_signaling->handleIncomingPacket(
                reinterpret_cast<const char *>(data), size);
            break;
        }
        case 5: {
            // encryptRawPacket then decryptRawPacket round-trip
            webrtc::CopyOnWriteBuffer buf(data, size);
            auto encrypted = state->conn_signaling->encryptRawPacket(buf);
            if (encrypted.has_value()) {
                state->conn_signaling->decryptRawPacket(*encrypted);
            }
            break;
        }
        case 6: {
            // prepareForSending then handleIncoming round-trip
            tgcalls::Message msg;
            msg.data = tgcalls::AudioDataMessage{webrtc::CopyOnWriteBuffer(data, size)};
            auto encrypted = state->conn_transport->prepareForSending(msg);
            if (encrypted.has_value()) {
                // Create a fresh connection as receiver
                auto keyArray = std::make_shared<std::array<uint8_t, 256>>();
                std::copy(kTestKeyData, kTestKeyData + 256, keyArray->begin());
                tgcalls::EncryptionKey recvKey(keyArray, true); // isOutgoing=true for receiver
                tgcalls::EncryptedConnection receiver(
                    tgcalls::EncryptedConnection::Type::Transport,
                    recvKey, [](int,int){});
                receiver.handleIncomingPacket(
                    reinterpret_cast<const char*>(encrypted->bytes.data()),
                    encrypted->bytes.size());
            }
            break;
        }
        case 7: {
            // Feed attacker-modified encrypted packet
            // Encrypt valid message, then corrupt some bytes, then try to decrypt
            tgcalls::Message msg;
            msg.data = tgcalls::RequestVideoMessage{};
            auto encrypted = state->conn_transport->prepareForSending(msg);
            if (encrypted.has_value() && !encrypted->bytes.empty()) {
                // Corrupt using fuzz data
                for (size_t i = 0; i < size && i < encrypted->bytes.size(); i++) {
                    encrypted->bytes[i] ^= data[i];
                }
                auto keyArray = std::make_shared<std::array<uint8_t, 256>>();
                std::copy(kTestKeyData, kTestKeyData + 256, keyArray->begin());
                tgcalls::EncryptionKey recvKey(keyArray, true);
                tgcalls::EncryptedConnection receiver(
                    tgcalls::EncryptedConnection::Type::Transport,
                    recvKey, [](int,int){});
                receiver.handleIncomingPacket(
                    reinterpret_cast<const char*>(encrypted->bytes.data()),
                    encrypted->bytes.size());
            }
            break;
        }
    }

    return 0;
}

#include "driver.h"
