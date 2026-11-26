# tgcalls Security Audit Report

**Repository:** TelegramMessenger/tgcalls  
**Commit:** 06992a41a1161ff60d7d5d3f42147e8867c4c6f0  
**Date:** 2026-11-26  
**Scope:** Telegram-authored C++ code in tgcalls/ (excluding upstream WebRTC/opus/libvpx/BoringSSL)  
**Methodology:** Manual code review + ASAN/UBSAN fuzzing (6 harnesses, seed corpus + ~8,900 mutation iterations)  
**Bug classes in scope:** OOB write (CWE-787), UAF (CWE-416), type confusion (CWE-843), integer overflow (CWE-190), OOB read info-leak (CWE-125), double-free (CWE-415), protocol/crypto breaks

---

## Executive Summary

The tgcalls codebase is **generally well-engineered** with consistent bounds checking at critical boundaries. The code uses `webrtc::ByteBufferReader` for safe deserialization, validates packet sizes before processing, and applies proper MAC verification before decryption.

**Fuzzing results:** Six ASAN+UBSAN harnesses targeting gzip decompression, Message binary TLV deserialization, EncryptedConnection packet handling, GroupFrameTransformer (H264/VP8 NAL parsing + encrypt/decrypt), JSON signaling, and parseHex were run with hand-crafted seed corpora (~100 seeds total) plus ~8,900 randomized mutation iterations using 6 mutation strategies (bit flip, truncate, append, insert, boundary values, random replacement). **Zero ASAN crashes (heap-buffer-overflow, stack-buffer-overflow, use-after-free, double-free) were detected.** One UBSAN finding was confirmed: null pointer passed to `memcpy` with size 0 in `DeserializeRawMessage` (technically UB, no security impact). No bounty-eligible memory corruption or exploitable protocol/crypto break was found through static analysis or dynamic testing.

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
bool ConstTimeIsDifferent(const void *a, const void *b, size_t size) {
    auto ca = reinterpret_cast<const char*>(a);
    auto cb = reinterpret_cast<const char*>(b);
    volatile auto different = false;
    for (const auto ce = ca + size; ca != ce; ++ca, ++cb) {
        different = different | (*ca != *cb);
    }
    return different;
}
```

**Analysis:** This function attempts constant-time comparison for MAC verification (used at lines 142, 189, 479 for msgKey verification in `decryptRawPacket`, `decryptFullPlaintextPacket`, and `handleIncomingPacket`) but has weaknesses:

1. `volatile auto different = false` deduces to `volatile bool`. The expression `different = different | (*ca != *cb)` evaluates `bool | bool`. Once `different` is `true`, a compiler may recognize that `true | X` is always `true` regardless of `X`, and skip evaluating the comparison `*ca != *cb` for remaining iterations. The `volatile` qualifier forces the load/store but does not prevent simplifying the computation itself.
2. The correct pattern uses `volatile unsigned char` with XOR accumulation: `diff |= (*ca ^ *cb)`, where `unsigned char | unsigned char` has no such identity simplification.
3. OpenSSL's `CRYPTO_memcmp` (available via the existing `<openssl/crypto.h>` include in `CryptoHelper.h`) handles this correctly.

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

### OBS-2: Undefined Behavior: null memcpy in DeserializeRawMessage
- **File:** `Message.cpp:393-396`
- **CWE:** CWE-476 (NULL Pointer Dereference)
- **Severity:** Informational
- **Attribution:** Telegram-authored

```cpp
webrtc::CopyOnWriteBuffer result;
result.SetSize(length);  // length can be 0
if (!reader.ReadBytes(webrtc::ArrayView<uint8_t>(
    (uint8_t *)result.MutableData(), result.size()))) {
```

**Analysis:** When `length == 0`, `SetSize(0)` creates a zero-sized buffer where `MutableData()` returns `nullptr`. The subsequent `ReadBytes` call passes `nullptr` to `memcpy` with size 0. Per C/C++ standard (7.24.1/2), passing a null pointer to `memcpy` is undefined behavior even when the count is zero. UBSAN confirms this finding.

**Exploitability:** No security impact. The UB is benign on all practical implementations (memcpy with count 0 is a no-op). A conforming compiler could theoretically optimize based on the UB assumption, but no known compiler does so for this pattern. **Not bounty-eligible.**

**Recommended fix:** Guard with `if (length > 0)` before the `ReadBytes` call, or initialize the buffer only when length is non-zero.

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

### OBS-6: Counter Space Exhaustion (via computeNextSeq)
- **File:** `EncryptedConnection.cpp:304-316`, counter management
- **CWE:** CWE-323 (Reusing a Nonce, Key Pair in Encryption)
- **Severity:** Informational (theoretical)
- **Attribution:** Telegram-authored

**Analysis:** The outgoing counter `_counter` is `uint32_t`, incremented by 1 per packet. The low 30 bits are the effective counter (top 2 bits are flag bits: `kSingleMessagePacketSeqBit = 1<<31`, `kMessageRequiresAckSeqBit = 1<<30`). `computeNextSeq()` correctly guards against exceeding `kMaxAllowedCounter` (0x3FFFFFFF), returning `std::nullopt` if reached.

At typical VoIP packet rates (~50 packets/sec), exhaustion takes ~248 days. The guard prevents overflow but provides no rekeying mechanism.

**Exploitability:** `computeNextSeq` correctly refuses further packets at the limit. Not exploitable. **Not bounty-eligible.**

---

### OBS-7: `encryptRawPacket` Bypasses Counter Limit Check
- **File:** `EncryptedConnection.cpp:102-118`
- **CWE:** CWE-323 (Reusing a Nonce, Key Pair in Encryption)
- **Severity:** Low (theoretical, impractical)
- **Attribution:** Telegram-authored

```cpp
std::optional<webrtc::CopyOnWriteBuffer> EncryptedConnection::encryptRawPacket(
    webrtc::CopyOnWriteBuffer const &buffer) {
    auto seq = ++_counter;  // No kMaxAllowedCounter check!
    // ...
    auto encryptedPacket = encryptPrepared(result);
    // ...
}
```

**Analysis:** Unlike `computeNextSeq()` (used by `prepareForSending` and `prepareForSendingRawMessage`), the `encryptRawPacket()` method directly increments `_counter` without checking `kMaxAllowedCounter`. After ~1.07 billion packets, `_counter` exceeds 0x3FFFFFFF and the seq value encroaches on the flag bits. The receiver's `CounterFromSeq()` masks the top 2 bits: `seq & 0x3FFFFFFF`, causing counter values to wrap to 0.

This creates two issues:
1. **Replay protection confusion:** Wrapped counter values (0, 1, 2, ...) duplicate values from the start of the connection. The receiver's `registerIncomingCounter` may reject legitimate new packets as replays, or accept actual replay packets if the sliding window has moved past the original usage.
2. **Shared counter:** Since `encryptRawPacket` and `computeNextSeq` share the same `_counter` field, overflow from `encryptRawPacket` also affects the counter space available to `prepareForSending`.

**Mitigating factors:**
- Not a nonce-reuse vulnerability: the AES key/IV is derived from `SHA256(key_portion + plaintext)`, so different plaintexts always produce different nonces regardless of counter value.
- Requires ~1.07 billion `encryptRawPacket` calls (~248 days at 50 pps). Real calls don't last this long.
- `encryptRawPacket` is a newer v2wasm seam function, likely called less frequently than the main packet path.

**Recommended fix:** Add the same `kMaxAllowedCounter` check from `computeNextSeq`:
```cpp
if (_counter == kMaxAllowedCounter) {
    return std::nullopt;
}
auto seq = ++_counter;
```

**Exploitability:** Impractical for real calls. **Not bounty-eligible.**

---

## Fuzzing Results

### Environment
- **Compiler:** GCC 13.3.0 with `-fsanitize=address,undefined -g -O1 -std=c++20`
- **Sanitizers:** AddressSanitizer (ASAN) + UndefinedBehaviorSanitizer (UBSAN)
- **Driver:** File-based harness driver (reads files from argv, feeds to `LLVMFuzzerTestOneInput`-style entry point)
- **Mutation fuzzer:** Shell-based mutation engine with 6 strategies: byte flip, truncation, append random, insert random, boundary values (all-zero/all-0xFF), random replacement
- **Seed corpus:** ~100 hand-crafted inputs targeting specific code paths
- **WebRTC stubs:** 20+ minimal header-only stubs for standalone compilation without full WebRTC build

### Targets and Results

| Harness | Target Code | Seeds | Mutations | ASAN Crashes | UBSAN Findings |
|---------|------------|-------|-----------|--------------|----------------|
| `harness_reflector` | `parseHex()` | 7 | 971 | 0 | 0 |
| `harness_gzip` | `gunzipData()`, `isGzip()` | 13 | 930 | 0 | 0 |
| `harness_message` | `DeserializeMessage()`, `DeserializeRawMessage()` (all 10 types) | 29 | 2,000 | 0 | 1 (OBS-2) |
| `harness_group_frame` | `calculateH264FramePlaintextHeaderSize()`, `calculateVp8FramePlaintextHeaderSize()`, `ValidateEncryptedFrame()`, encrypt/decrypt audio/video | 20 | 2,000 | 0 | 0 |
| `harness_encrypted` | `handleIncomingPacket()`, `handleIncomingRawPacket()`, `decryptRawPacket()`, `decryptFullPlaintextPacket()`, encrypt-then-corrupt-then-decrypt | 17 | 2,000 | 0 | 0 |
| `harness_signaling` | json11 parsing + Signaling.cpp field access patterns (InitialSetup, Candidates, NegotiateChannels, MediaState) | 24 | 1,000 | 0 | 0 |
| **Total** | | **110** | **~8,900** | **0** | **1** |

### UBSAN Finding: OBS-2 (null memcpy)

The message harness triggers UBSAN on `DeserializeRawMessage` when the length field is 0:
```
runtime error: null pointer passed as argument 1, which is declared to never be null
```
This occurs at `Message.cpp:395` where `MutableData()` returns nullptr for a zero-sized `CopyOnWriteBuffer`, and the resulting nullptr is passed to `memcpy` with count 0. Technically UB per C standard, but no security impact.

### Harnesses and Build

Fuzzing harnesses, stubs, and mutation script at `/home/user/tgcalls-audit/fuzz-build/`:
- `harness_reflector.cpp` - parseHex standalone function
- `harness_gzip.cpp` - gzip decompression with multiple size limits
- `harness_message.cpp` - Message TLV deserialization (selector-based: single/multi, typed/raw)
- `harness_group_frame.cpp` - GroupFrameTransformer: H264/VP8 NAL parsing, ValidateEncryptedFrame, encrypt/decrypt
- `harness_encrypted.cpp` - EncryptedConnection: all 4 decrypt entry points + encrypt-then-corrupt-then-decrypt
- `harness_signaling.cpp` - json11 parsing with Signaling.cpp field access patterns
- `driver.h` - File-based fuzz driver
- `mutate_and_fuzz.sh` - Shell mutation fuzzer
- `stubs/` - 20+ minimal WebRTC type stubs for standalone compilation
- `corpus/` - Seed corpora organized by harness

---

## Recommendations

1. **Replace `ConstTimeIsDifferent` with `CRYPTO_memcmp`** (already available via `<openssl/crypto.h>` in `CryptoHelper.h`). One-line fix that eliminates the timing concern.

2. **Guard zero-length in `DeserializeRawMessage`:** Add `if (length == 0) return webrtc::CopyOnWriteBuffer();` before `SetSize(length)` to avoid the null-memcpy UB.

3. **Add `kMaxAllowedCounter` check to `encryptRawPacket`:** Mirror the guard from `computeNextSeq()` to prevent counter overflow in the v2wasm code path.

4. **Validate `parseHex` input** for even length and valid hex characters.

5. **Run continuous fuzzing** on the harnesses provided, particularly the post-decryption parsing paths (`harness_encrypted`, `harness_message`, `harness_group_frame`). The seed corpora and mutation script are designed for easy integration with a CI fuzzing pipeline.

---

## Triage Summary

| Finding | CWE | Telegram-authored | Memory Corruption | Security Primitive | Bounty Eligible |
|---------|-----|-------------------|--------------------|--------------------|-----------------|
| OBS-1 Timing MAC | CWE-208 | Yes | No | Timing side-channel | No (impractical) |
| OBS-2 Null memcpy UB | CWE-476 | Yes | No | No | No |
| OBS-3 Alloc without limit | CWE-770 | Yes | No | No | No (DoS only) |
| OBS-4 parseHex validation | CWE-20 | Yes | No | No | No |
| OBS-5 DTLS fingerprint | N/A | Yes (by design) | No | No | No |
| OBS-6 Counter exhaustion | CWE-323 | Yes | No | Counter replay | No (guarded) |
| OBS-7 encryptRawPacket counter bypass | CWE-323 | Yes | No | Counter replay | No (impractical) |

**None of the 7 findings from static analysis or dynamic fuzzing (6 ASAN+UBSAN harnesses) meet Telegram's bug bounty threshold of "memory corruption with a security primitive" or "exploitable protocol/crypto break."**
