# tgcalls Security Audit Report

**Repository:** TelegramMessenger/tgcalls  
**Commit:** 06992a41a1161ff60d7d5d3f42147e8867c4c6f0  
**Date:** 2026-11-26  
**Scope:** Telegram-authored C++ code in tgcalls/ (excluding upstream WebRTC/opus/libvpx/BoringSSL)  
**Methodology:** Manual code review + fuzzing harness design  
**Bug classes in scope:** OOB write (CWE-787), UAF (CWE-416), type confusion (CWE-843), integer overflow (CWE-190), OOB read info-leak (CWE-125), double-free (CWE-415), protocol/crypto breaks

---

## Executive Summary

The tgcalls codebase is **generally well-engineered** with consistent bounds checking at critical boundaries. The code uses `webrtc::ByteBufferReader` for safe deserialization, validates packet sizes before processing, and applies proper MAC verification before decryption. No bounty-eligible memory corruption with a security primitive or exploitable protocol/crypto break was confirmed through static analysis alone. Several observations warrant continued fuzzing and monitoring.

---

## Attack Surface Map

### 1. EncryptedConnection (Telegram-authored, CRITICAL)
- **Files:** `EncryptedConnection.cpp` (809 lines), `EncryptedConnection.h` (113 lines)
- **Role:** E2E encryption layer for 1:1 calls (AES-256-CTR + SHA256 MAC, MTProto-style key derivation)
- **Entry points:** `handleIncomingPacket()`, `handleIncomingRawPacket()`, `decryptRawPacket()`, `decryptFullPlaintextPacket()`
- **Threat model:** Network attacker (without key) or malicious peer (with shared key)

### 2. Message Deserialization (Telegram-authored, HIGH)
- **Files:** `Message.cpp` (402 lines), `Message.h` (149 lines)
- **Role:** Binary TLV message parsing after decryption
- **Entry points:** `DeserializeMessage()`, `DeserializeRawMessage()`
- **Threat model:** Malicious peer providing crafted plaintext after E2E decryption

### 3. ReflectorPort (Telegram-authored, HIGH)
- **Files:** `v2/ReflectorPort.cpp` (955 lines), `v2/ReflectorPort.h` (243 lines)
- **Role:** Telegram's custom relay/reflector transport (replaces TURN)
- **Entry points:** `HandleIncomingPacket()`
- **Threat model:** Network attacker sending crafted reflector packets

### 4. Signaling (Telegram-authored, MEDIUM)
- **Files:** `v2/Signaling.cpp` (986 lines), `v2_4_0_0/Signaling_4_0_0.cpp` (703 lines)
- **Role:** JSON-based call signaling via json11
- **Entry points:** `Message::parse()`, `convertSignalingContentToContentInfo()`
- **Threat model:** Peer/MitM injecting malformed signaling JSON (after E2E decryption)

### 5. GroupFrameTransformer (Telegram-authored, MEDIUM)
- **Files:** `group/GroupFrameTransformer.cpp` (574 lines)
- **Role:** Group call E2E frame encryption/decryption, H264 NAL parsing
- **Entry points:** `decryptGroupVideoFrame()`, `decryptGroupAudioFrame()`, `calculateH264FramePlaintextHeaderSize()`
- **Threat model:** Malicious group call participant sending crafted video/audio frames

### 6. MtProtoDtlsTransport (Telegram-authored, MEDIUM)
- **Files:** `v2/MtProtoDtlsTransport.cpp` (304 lines)
- **Role:** Replaces standard DTLS with MTProto encryption over ICE
- **Entry points:** `onIceReadPacket()`, `emitDecryptedMessage()`
- **Threat model:** Network-level packet injection (requires encryption key)

### 7. WASM/Compat Signaling Framing (Telegram-authored, LOW)
- **Files:** `v2wasm/SignalingFraming.cpp` (61 lines), `v2/CompatSignalingFraming.h` (47 lines)
- **Role:** Signaling framing with gzip compression
- **Entry points:** `receivePacket()`
- **Threat model:** Peer sending compressed signaling (zip bomb guarded at 2MB)

### 8. Gzip Utilities (Telegram-authored, LOW)
- **Files:** `utils/gzip.cpp` (103 lines)
- **Role:** Compression/decompression with size limits
- **Entry points:** `gunzipData()`
- **Threat model:** Decompression bomb (guarded by sizeLimit parameter)

---

## Findings

### OBS-1: Weak Constant-Time MAC Comparison
- **File:** `EncryptedConnection.cpp:63-71`
- **CWE:** CWE-208 (Observable Timing Discrepancy)
- **Severity:** Informational (crypto hygiene)
- **Attribution:** Telegram-authored

```cpp
static bool ConstTimeIsDifferent(const void *a, const void *b, size_t size) {
    volatile auto different = false;
    const auto a1 = reinterpret_cast<const char *>(a);
    const auto b1 = reinterpret_cast<const char *>(b);
    for (size_t i = 0; i < size; ++i) {
        if (a1[i] != b1[i]) {
            different = true;
        }
    }
    return different;
}
```

**Analysis:** This function attempts constant-time comparison for MAC verification but has three weaknesses:

1. `volatile auto different = false` uses `bool` type. A compiler may optimize the `if` branch since `true OR true == true` (the `volatile` qualifier only prevents eliding stores to `different`, not the comparison branch).
2. The per-byte comparison `a1[i] != b1[i]` generates a conditional branch in the compiled code. The `volatile` store target prevents the loop from short-circuiting, but the branch predictor still leaks timing information.
3. `volatile` on a local variable is compiler-dependent and not a formal memory barrier.

**Recommended fix:** Use `CRYPTO_memcmp()` from BoringSSL (already a dependency) or accumulate XOR results in a `volatile unsigned char`:
```cpp
volatile unsigned char diff = 0;
for (size_t i = 0; i < size; ++i) {
    diff |= (unsigned char)(a1[i] ^ b1[i]);
}
return diff != 0;
```

**Exploitability:** Very difficult over a network. Requires sub-microsecond timing precision, many attempts with the same key, and controlled network jitter. The MAC is 16 bytes of SHA256 output, so the timing window per byte is extremely narrow. **Not bounty-eligible as a standalone finding.**

---

### OBS-2: Undefined Behavior from Unaligned Memory Access
- **File:** `EncryptedConnection.cpp:48-50`
- **CWE:** CWE-704 (Incorrect Type Conversion or Cast)
- **Severity:** Low
- **Attribution:** Telegram-authored

```cpp
static uint32_t ReadSeq(const void *ptr) {
    return *reinterpret_cast<const uint32_t*>(ptr);
}
```

**Analysis:** `reinterpret_cast` to `uint32_t*` on a potentially unaligned pointer is undefined behavior per C++ standard. On x86/x64 this works silently (possibly with performance penalty). On ARM/MIPS with strict alignment requirements, this can cause SIGBUS. UBSAN would flag this.

**Additionally:** This reads in host byte order, not network byte order. The counter is written via `ByteBufferWriter::WriteUInt32()` which uses big-endian (network order), but `ReadSeq` reads in native endianness. This is consistent only on big-endian systems. However, examining the usage: `ReadSeq` is called at line 464 on `fullBuffer.data()` which was produced by the sender's `ByteBufferWriter::WriteUInt32()`. The WebRTC `ByteBufferWriter/Reader` use big-endian, but `ReadSeq` uses native endian. This is an inconsistency -- **but it works in practice because both sender and receiver use the same `ReadSeq` and the same endianness, so the counter values are consistent within a session on a given platform.**

Cross-platform calls (e.g., ARM phone to x86 server if intermediate relay decrypts) would have the counter interpreted differently. However, the reflector does not decrypt packets (only routes based on peer_tag), so in practice this works.

**Recommended fix:** Use `memcpy` + `ntohl`:
```cpp
static uint32_t ReadSeq(const void *ptr) {
    uint32_t val;
    memcpy(&val, ptr, 4);
    return val; // keep native endian if both sides agree
}
```

**Exploitability:** No security impact on x86/x64. SIGBUS crash on some ARM targets (DoS only). **Not bounty-eligible.**

---

### OBS-3: Attacker-Controlled Heap Allocation via DeserializeRawMessage
- **File:** `Message.cpp:377-400`
- **CWE:** CWE-770 (Allocation of Resources Without Limits)
- **Severity:** Low (DoS only, requires encryption key)
- **Attribution:** Telegram-authored

```cpp
uint32_t length = 0;
if (!reader.ReadUInt32(&length)) {
    return std::nullopt;
}
if (length > 1024 * 1024) {
    return std::nullopt;
}
webrtc::CopyOnWriteBuffer result;
result.SetSize(length);       // <-- allocation happens here
if (!reader.ReadBytes(...)) {  // <-- fails if length > available data
    return std::nullopt;
}
```

**Analysis:** A malicious peer (who possesses the shared encryption key) can craft a packet where the decrypted payload contains a 4-byte length field claiming up to 1MB. The allocation at `SetSize(length)` happens BEFORE `ReadBytes` validates that sufficient data is available. If the length exceeds the reader's remaining data, `ReadBytes` returns false and the buffer is freed -- but the allocation was performed.

However, the outer packet size is bounded by `kMaxIncomingPacketSize = 128 * 1024` for transport and `kMaxSignalingPacketSize = 16 * 1024` for signaling. The 1MB limit is thus never reached in practice since the reader can never have more than ~128KB. The allocation for `length` values between the actual data size and 1MB will always be followed by a failed `ReadBytes` and immediate deallocation.

**Exploitability:** Requires the encryption key (malicious peer). Impact is temporary allocation of up to 1MB per packet, with immediate deallocation. At typical packet rates, memory pressure is minimal. **Not bounty-eligible (DoS only).**

---

### OBS-4: parseHex Missing Validation
- **File:** `v2/ReflectorPort.cpp:32-42`
- **CWE:** CWE-20 (Improper Input Validation)
- **Severity:** Informational
- **Attribution:** Telegram-authored

**Analysis:** The `parseHex` function does not validate that the input string has even length or contains only valid hex characters. An odd-length string silently drops the last character. Invalid hex characters produce implementation-defined byte values (depending on how the hex-to-nibble conversion is implemented).

The result is used to construct `peer_tag_`. If `rawPeerTag.size() != 16`, the fallback creates a 16-byte zero tag, then appends 4 bytes of `randomTag_`, producing a 20-byte `peer_tag_`. This 20-byte tag is later used in `SendTo()` where `peer_tag_.size() - 4 = 16` bytes are sent as the outgoing tag, which is 4 bytes too many for the protocol.

**Exploitability:** Requires the server to supply a malformed hex password. This is a configuration-path bug, not reachable by a network attacker. **Not bounty-eligible.**

---

### OBS-5: DTLS Fingerprint Intentionally Discarded
- **File:** `v2/MtProtoDtlsTransport.cpp:199-208`
- **CWE:** N/A (by design)
- **Severity:** Informational (design observation)
- **Attribution:** Telegram-authored

```cpp
webrtc::RTCError MtProtoDtlsTransport::SetRemoteParameters(...) {
    // The fingerprint is accepted and discarded, never verified: mtproto's
    // shared key is the authentication.
    if (role) { _dtlsRole = role; }
    return webrtc::RTCError::OK();
}
```

**Analysis:** This is an intentional design decision: in MTProto mode, the DTLS fingerprint is replaced by the shared MTProto key for authentication. The comment explicitly states the rationale. The security of the call relies on the MTProto key exchange (derived from Diffie-Hellman via the Telegram server, with emoji verification for E2E).

This is not a vulnerability -- it's a documented alternative authentication path. Standard DTLS-SRTP is used for group calls (`GroupNetworkManager.cpp`).

---

### OBS-6: Counter Space Exhaustion / Nonce Reuse
- **File:** `EncryptedConnection.cpp`, counter management
- **CWE:** CWE-323 (Reusing a Nonce, Key Pair in Encryption)
- **Severity:** Informational (theoretical)
- **Attribution:** Telegram-authored

**Analysis:** The outgoing counter `_counter` is `uint32_t`, incremented by 1 per packet. The low 30 bits are the effective counter (top 2 bits are flag bits: `kSingleMessagePacketSeqBit = 1<<30`, `kMessageRequiresAckSeqBit = 1<<31`). After 2^30 (~1.07 billion) packets, the counter bits overflow into the flag bit area.

At typical VoIP packet rates (~50 packets/sec), exhaustion takes ~248 days of continuous calling. At that point, nonce reuse in AES-CTR would leak XOR of plaintexts. The incoming counter validation (`registerIncomingCounter`) uses the same 30-bit space.

**Exploitability:** Impractical for real calls. No guard exists, but the call would need to run for ~8 months continuously. **Not bounty-eligible.**

---

## Areas Requiring Fuzzing Validation

The following code paths are well-defended in static review but would benefit from coverage-guided fuzzing to detect edge cases:

1. **EncryptedConnection::processPacket / processRawPacket** - Multi-message parsing loop with seq/type/ack handling. Fuzz with known encryption key to exercise post-decryption parsing.

2. **Message deserialization (all 10 message types)** - Binary TLV parsing. Fuzz `DeserializeMessage` and `DeserializeRawMessage` directly.

3. **GroupFrameTransformer H264 NAL parsing** - `calculateH264FramePlaintextHeaderSize` with crafted NAL structures. Complex bit-level parsing is historically error-prone.

4. **ReflectorPort::HandleIncomingPacket** - Reflector packet parsing with peer_tag validation and special packet detection.

5. **Signaling JSON parsing** - `signaling::Message::parse` and `signaling_4_0_0::Message::parse` with crafted JSON.

6. **gunzipData** - Decompression with crafted gzip streams near size limits.

---

## Fuzzing Harnesses

See `harnesses/` directory for libFuzzer-compatible harnesses targeting each entry point. These require compilation against the tgcalls + WebRTC build system with:

```
-fsanitize=address,undefined,fuzzer
-DADDRESS_SANITIZER -DUNDEFINED_SANITIZER
```

Build instructions are in `scripts/build_fuzzers.sh`.

---

## Recommendations

1. **Replace `ConstTimeIsDifferent` with `CRYPTO_memcmp`** (BoringSSL) or OpenSSL's `CRYPTO_memcmp`. This is a one-line fix that eliminates the timing concern.

2. **Fix `ReadSeq` unaligned access** using `memcpy` instead of `reinterpret_cast`. UBSAN will flag this.

3. **Add counter exhaustion guard** in `EncryptedConnection`: reject outgoing packets when counter approaches 2^30. Log a warning and require rekeying.

4. **Validate `parseHex` input** for even length and valid hex characters.

5. **Run continuous fuzzing** on the harnesses provided, particularly the post-decryption parsing paths.

---

## Triage Summary

| Finding | CWE | Telegram-authored | Memory Corruption | Security Primitive | Bounty Eligible |
|---------|-----|-------------------|--------------------|--------------------|-----------------|
| OBS-1 Timing MAC | CWE-208 | Yes | No | Timing side-channel | No (impractical) |
| OBS-2 Unaligned access | CWE-704 | Yes | No (crash) | No | No (DoS only) |
| OBS-3 Alloc without limit | CWE-770 | Yes | No | No | No (DoS only) |
| OBS-4 parseHex validation | CWE-20 | Yes | No | No | No |
| OBS-5 DTLS fingerprint | N/A | Yes (by design) | No | No | No |
| OBS-6 Counter exhaustion | CWE-323 | Yes | No | Nonce reuse | No (impractical) |

**None of the findings from static analysis alone meet Telegram's bug bounty threshold of "memory corruption with a security primitive" or "exploitable protocol/crypto break." Fuzzing is required to surface potential memory corruption bugs that static review cannot detect.**
