// Standalone fuzz harness for tgcalls H264/VP8 frame parsing
// Extracts the parsing logic from GroupFrameTransformer.cpp into a standalone target
// to avoid heavy WebRTC class dependencies (FrameTransformerInterface, Mutex, etc.)

#include <algorithm>
#include <cassert>
#include <cstring>
#include <optional>
#include <vector>
#include <cstdint>

#include "common_video/h264/h264_common.h"
#include "rtc_base/bitstream_reader.h"
#include "rtc_base/logging.h"
#include "api/array_view.h"
#include "drivers/fuzz_driver.h"

namespace tgcalls_fuzz {

static constexpr uint8_t kTypeMask = 0x1F;
static constexpr uint8_t kFuA = 28;
static constexpr uint8_t kIdr = 5;
static constexpr uint8_t kSps = 7;
static constexpr uint8_t kPps = 8;
static constexpr uint8_t kSei = 6;
static constexpr uint8_t kStapA = 24;
static constexpr size_t kNalShortStartCode = 3;
static constexpr size_t kNalHeaderSize = 1;
static constexpr size_t kFuAHeaderSize = 2;
constexpr size_t kLengthFieldSize = 2;
constexpr size_t kStapAHeaderSize = kNalHeaderSize + kLengthFieldSize;
constexpr uint8_t P_BIT = 0x01;
constexpr uint8_t kH26XNaluShortStartSequenceSize = 3;

using IndexStartCodeSizePair = std::pair<size_t, size_t>;

// Copied from GroupFrameTransformer.cpp - this is the code under test
size_t calculateSliceHeaderBytesForPpsId(const uint8_t* data, size_t size) {
    if (size < 2)
        return 0;

    std::vector<uint8_t> rbsp = webrtc::H264::ParseRbsp(data, size);
    if (rbsp.size() < 2)
        return 0;

    webrtc::ArrayView<const uint8_t> rbspView(rbsp.data() + 1, rbsp.size() - 1);
    webrtc::BitstreamReader reader(rbspView);

    reader.ReadExponentialGolomb();
    if (!reader.Ok()) return 4;

    reader.ReadExponentialGolomb();
    if (!reader.Ok()) return 4;

    reader.ReadExponentialGolomb();
    if (!reader.Ok()) return 4;

    size_t bitsConsumed = rbspView.size() * 8 - reader.RemainingBitCount();
    size_t bytesRead = 1 + (bitsConsumed + 7) / 8;
    return bytesRead + 1;
}

std::optional<IndexStartCodeSizePair> FindNextH26XNaluIndex(const uint8_t* buffer,
                                                            const size_t bufferSize,
                                                            const size_t searchStartIndex = 0) {
    constexpr uint8_t kH26XStartCodeHighestPossibleValue = 1;
    constexpr uint8_t kH26XStartCodeEndByteValue = 1;
    constexpr uint8_t kH26XStartCodeLeadingBytesValue = 0;

    if (bufferSize < kH26XNaluShortStartSequenceSize) {
        return std::nullopt;
    }

    for (size_t i = searchStartIndex; i < bufferSize - kH26XNaluShortStartSequenceSize;) {
        if (buffer[i + 2] > kH26XStartCodeHighestPossibleValue) {
            i += kH26XNaluShortStartSequenceSize;
        }
        else if (buffer[i + 2] == kH26XStartCodeEndByteValue) {
            if (buffer[i + 1] == kH26XStartCodeLeadingBytesValue &&
                buffer[i] == kH26XStartCodeLeadingBytesValue) {
                auto nalUnitStartIndex = i + kH26XNaluShortStartSequenceSize;
                if (i >= 1 && buffer[i - 1] == kH26XStartCodeLeadingBytesValue) {
                    return std::optional<IndexStartCodeSizePair>({nalUnitStartIndex, 4});
                } else {
                    return std::optional<IndexStartCodeSizePair>({nalUnitStartIndex, 3});
                }
            }
            i += kH26XNaluShortStartSequenceSize;
        }
        else {
            ++i;
        }
    }
    return std::nullopt;
}

std::vector<uint8_t> calculateH264FramePlaintextHeaderSize(webrtc::ArrayView<const uint8_t> frame, uint32_t& headerSize) {
    if (frame.empty()) {
        headerSize = 0;
        return std::vector<uint8_t>();
    }

    std::vector<webrtc::H264::NaluIndex> naluIndices =
        webrtc::H264::FindNaluIndices(frame.data(), frame.size());

    if (naluIndices.empty()) {
        headerSize = 0;
        std::vector<uint8_t> frameData;
        frameData.resize(frame.size());
        std::copy(frame.begin(), frame.end(), frameData.begin());
        return frameData;
    }

    size_t maxOffset = 0;
    std::vector<size_t> naluToUpdate;

    for (const auto& naluIndex : naluIndices) {
        size_t startCodeLength = naluIndex.payload_start_offset - naluIndex.start_offset;

        if (startCodeLength == kNalShortStartCode) {
            naluToUpdate.push_back(naluIndex.start_offset);
        }

        size_t headerEndOffset = naluIndex.payload_start_offset + kNalHeaderSize;

        if (naluIndex.payload_size >= kNalHeaderSize) {
            uint8_t nalType = frame[naluIndex.payload_start_offset] & kTypeMask;

            if (nalType == kFuA) {
                if (naluIndex.payload_size >= kFuAHeaderSize) {
                    headerEndOffset = naluIndex.payload_start_offset + kFuAHeaderSize;
                    bool isStartBit = (frame[naluIndex.payload_start_offset + 1] & 0x80) != 0;
                    if (isStartBit) {
                        uint8_t originalNalType = frame[naluIndex.payload_start_offset + 1] & kTypeMask;
                        if (originalNalType == kIdr || originalNalType == 1) {
                            headerEndOffset += 4;
                        }
                    }
                }
            } else if (nalType == kStapA) {
                if (naluIndex.payload_size >= kStapAHeaderSize) {
                    headerEndOffset = naluIndex.payload_start_offset + kStapAHeaderSize;
                    if (naluIndex.payload_size > kStapAHeaderSize) {
                        uint8_t firstNalType = frame[naluIndex.payload_start_offset + kStapAHeaderSize] & kTypeMask;
                        if (firstNalType == kIdr || firstNalType == 1) {
                            headerEndOffset += 4;
                        }
                    }
                }
            }
            else if (nalType == kIdr || nalType == 1) {
                size_t ppsIdBytes = calculateSliceHeaderBytesForPpsId(
                    frame.data() + naluIndex.payload_start_offset,
                    naluIndex.payload_size);
                headerEndOffset = naluIndex.payload_start_offset + ppsIdBytes;
                maxOffset = std::max(maxOffset, headerEndOffset);
                break;
            }
            else if (nalType == kSps || nalType == kPps || nalType == kSei) {
                headerEndOffset = naluIndex.payload_start_offset + naluIndex.payload_size;
            }
        }

        maxOffset = std::max(maxOffset, headerEndOffset);
    }

    std::vector<uint8_t> frameData;
    frameData.resize(frame.size() + naluToUpdate.size());

    size_t offset = 0;
    for (size_t i = 0; i < naluToUpdate.size(); ++i) {
        const auto& naluIndex = naluToUpdate[i];
        if (naluIndex - offset > 0) {
            std::copy(frame.begin() + offset, frame.begin() + naluIndex, frameData.begin() + offset + i);
        }
        frameData[naluIndex + i] = 0;
        offset = naluIndex;
    }

    if (offset < frame.size()) {
        std::copy(frame.begin() + offset, frame.end(), frameData.begin() + offset + naluToUpdate.size());
    }

    headerSize = static_cast<uint32_t>(maxOffset + naluToUpdate.size());
    return frameData;
}

std::vector<uint8_t> calculateVp8FramePlaintextHeaderSize(webrtc::ArrayView<const uint8_t> frame, uint32_t& headerSize) {
    if (frame.empty()) {
        headerSize = 0;
        return std::vector<uint8_t>();
    }

    uint8_t first_byte = frame[0];
    bool is_key_frame = (first_byte & P_BIT) == 0;

    if (is_key_frame) {
        headerSize = frame.size() >= 10 ? 10 : ((uint32_t)frame.size());
    } else {
        headerSize = 1;
    }

    std::vector<uint8_t> frameData;
    frameData.resize(frame.size());
    std::copy(frame.begin(), frame.end(), frameData.begin());
    return frameData;
}

} // namespace tgcalls_fuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    uint8_t mode = data[0];
    data++;
    size--;

    webrtc::ArrayView<const uint8_t> frame(const_cast<uint8_t*>(data), size);

    switch (mode & 0x03) {
        case 0: {
            // Test H264 header calculation
            uint32_t headerSize = 0;
            tgcalls_fuzz::calculateH264FramePlaintextHeaderSize(frame, headerSize);
            break;
        }
        case 1: {
            // Test VP8 header calculation
            uint32_t headerSize = 0;
            tgcalls_fuzz::calculateVp8FramePlaintextHeaderSize(frame, headerSize);
            break;
        }
        case 2: {
            // Test slice header PPS ID parsing
            tgcalls_fuzz::calculateSliceHeaderBytesForPpsId(data, size);
            break;
        }
        case 3: {
            // Test NALU index finding
            tgcalls_fuzz::FindNextH26XNaluIndex(data, size, 0);
            if (size > 4) {
                tgcalls_fuzz::FindNextH26XNaluIndex(data, size, size / 2);
            }
            break;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    return fuzz::runDriver(argc, argv, 65536);
}
