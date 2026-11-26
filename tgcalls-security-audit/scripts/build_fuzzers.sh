#!/bin/bash
# Build script for tgcalls security fuzzing harnesses
#
# Prerequisites:
# 1. WebRTC source tree built with:
#    gn gen out/fuzz --args='
#      is_debug=true
#      is_asan=true
#      is_ubsan_security=true
#      use_libfuzzer=true
#      is_component_build=false
#    '
#    ninja -C out/fuzz
#
# 2. tgcalls source at $TGCALLS_SRC
# 3. WebRTC source at $WEBRTC_SRC
# 4. WebRTC build output at $WEBRTC_OUT

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HARNESS_DIR="${SCRIPT_DIR}/../harnesses"
OUTPUT_DIR="${SCRIPT_DIR}/../build"

# Adjust these paths for your environment
TGCALLS_SRC="${TGCALLS_SRC:-/path/to/tgcalls/tgcalls}"
WEBRTC_SRC="${WEBRTC_SRC:-/path/to/webrtc/src}"
WEBRTC_OUT="${WEBRTC_OUT:-/path/to/webrtc/src/out/fuzz}"

CC="${CC:-clang}"
CXX="${CXX:-clang++}"

SANITIZER_FLAGS="-fsanitize=address,undefined,fuzzer"
COMMON_FLAGS="-std=c++17 -g -O1 ${SANITIZER_FLAGS}"
COMMON_FLAGS+=" -DWEBRTC_POSIX -DWEBRTC_LINUX"

INCLUDES="-I${TGCALLS_SRC} -I${WEBRTC_SRC} -I${WEBRTC_SRC}/third_party/abseil-cpp"

# WebRTC libraries (adjust based on your build)
WEBRTC_LIBS="${WEBRTC_OUT}/obj/libwebrtc.a"
SYSTEM_LIBS="-lssl -lcrypto -lz -lpthread -ldl"

mkdir -p "${OUTPUT_DIR}"

echo "=== Building tgcalls fuzzing harnesses ==="
echo "TGCALLS_SRC: ${TGCALLS_SRC}"
echo "WEBRTC_SRC:  ${WEBRTC_SRC}"
echo "WEBRTC_OUT:  ${WEBRTC_OUT}"
echo ""

# 1. Build gzip harness (minimal dependencies)
echo "[1/6] Building fuzz_gzip..."
${CXX} ${COMMON_FLAGS} ${INCLUDES} \
    "${HARNESS_DIR}/fuzz_gzip.cpp" \
    "${TGCALLS_SRC}/utils/gzip.cpp" \
    -lz \
    -o "${OUTPUT_DIR}/fuzz_gzip" 2>&1 || echo "FAILED: fuzz_gzip"

# 2. Build signaling parse harness
echo "[2/6] Building fuzz_signaling_parse..."
${CXX} ${COMMON_FLAGS} ${INCLUDES} \
    "${HARNESS_DIR}/fuzz_signaling_parse.cpp" \
    "${TGCALLS_SRC}/v2/Signaling.cpp" \
    "${TGCALLS_SRC}/v2_4_0_0/Signaling_4_0_0.cpp" \
    "${TGCALLS_SRC}/third-party/json11.cpp" \
    ${WEBRTC_LIBS} ${SYSTEM_LIBS} \
    -o "${OUTPUT_DIR}/fuzz_signaling_parse" 2>&1 || echo "FAILED: fuzz_signaling_parse"

# 3. Build message deserialize harness
echo "[3/6] Building fuzz_message_deserialize..."
${CXX} ${COMMON_FLAGS} ${INCLUDES} \
    "${HARNESS_DIR}/fuzz_message_deserialize.cpp" \
    "${TGCALLS_SRC}/Message.cpp" \
    ${WEBRTC_LIBS} ${SYSTEM_LIBS} \
    -o "${OUTPUT_DIR}/fuzz_message_deserialize" 2>&1 || echo "FAILED: fuzz_message_deserialize"

# 4. Build encrypted connection harness
echo "[4/6] Building fuzz_encrypted_connection..."
${CXX} ${COMMON_FLAGS} ${INCLUDES} \
    "${HARNESS_DIR}/fuzz_encrypted_connection.cpp" \
    "${TGCALLS_SRC}/EncryptedConnection.cpp" \
    "${TGCALLS_SRC}/CryptoHelper.cpp" \
    "${TGCALLS_SRC}/Message.cpp" \
    ${WEBRTC_LIBS} ${SYSTEM_LIBS} \
    -o "${OUTPUT_DIR}/fuzz_encrypted_connection" 2>&1 || echo "FAILED: fuzz_encrypted_connection"

# 5. Build reflector port harness
echo "[5/6] Building fuzz_reflector_port..."
${CXX} ${COMMON_FLAGS} ${INCLUDES} \
    "${HARNESS_DIR}/fuzz_reflector_port.cpp" \
    ${WEBRTC_LIBS} ${SYSTEM_LIBS} \
    -o "${OUTPUT_DIR}/fuzz_reflector_port" 2>&1 || echo "FAILED: fuzz_reflector_port"

# 6. Build group frame harness
echo "[6/6] Building fuzz_group_frame..."
${CXX} ${COMMON_FLAGS} ${INCLUDES} \
    "${HARNESS_DIR}/fuzz_group_frame.cpp" \
    "${TGCALLS_SRC}/group/GroupFrameTransformer.cpp" \
    ${WEBRTC_LIBS} ${SYSTEM_LIBS} \
    -o "${OUTPUT_DIR}/fuzz_group_frame" 2>&1 || echo "FAILED: fuzz_group_frame"

echo ""
echo "=== Build complete ==="
echo "Harnesses in: ${OUTPUT_DIR}/"
echo ""
echo "Run each fuzzer:"
echo "  ${OUTPUT_DIR}/fuzz_gzip corpus_gzip/"
echo "  ${OUTPUT_DIR}/fuzz_signaling_parse corpus_signaling/"
echo "  ${OUTPUT_DIR}/fuzz_message_deserialize corpus_message/"
echo "  ${OUTPUT_DIR}/fuzz_encrypted_connection corpus_encrypted/"
echo "  ${OUTPUT_DIR}/fuzz_reflector_port corpus_reflector/"
echo "  ${OUTPUT_DIR}/fuzz_group_frame corpus_h264/"
echo ""
echo "Seed corpus generation:"
echo "  Record real call packets with tcpdump and extract payloads."
echo "  For signaling: capture JSON messages from debug logs."
echo "  For H264: extract raw H264 NAL units from group call recordings."
