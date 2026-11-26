#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "api/array_view.h"

namespace webrtc {
namespace H264 {

struct NaluIndex {
    size_t start_offset;
    size_t payload_start_offset;
    size_t payload_size;
};

inline std::vector<NaluIndex> FindNaluIndicesImpl(const uint8_t* data, size_t size) {
    std::vector<NaluIndex> indices;
    if (size < 4) return indices;
    size_t i = 0;
    while (i + 2 < size) {
        if (data[i] == 0 && data[i+1] == 0) {
            size_t start = i;
            size_t payload_start;
            if (i + 2 < size && data[i+2] == 1) {
                payload_start = i + 3;
            } else if (i + 3 < size && data[i+2] == 0 && data[i+3] == 1) {
                payload_start = i + 4;
            } else {
                ++i;
                continue;
            }
            if (!indices.empty()) {
                indices.back().payload_size = start - indices.back().payload_start_offset;
            }
            NaluIndex idx;
            idx.start_offset = start;
            idx.payload_start_offset = payload_start;
            idx.payload_size = 0;
            indices.push_back(idx);
            i = payload_start;
        } else {
            ++i;
        }
    }
    if (!indices.empty()) {
        indices.back().payload_size = size - indices.back().payload_start_offset;
    }
    return indices;
}

inline std::vector<NaluIndex> FindNaluIndices(const uint8_t* data, size_t size) {
    return FindNaluIndicesImpl(data, size);
}

inline std::vector<NaluIndex> FindNaluIndices(webrtc::ArrayView<const uint8_t> view) {
    return FindNaluIndicesImpl(view.data(), view.size());
}

inline std::vector<uint8_t> ParseRbsp(const uint8_t* data, size_t size) {
    std::vector<uint8_t> out;
    out.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        if (i + 2 < size && data[i] == 0 && data[i+1] == 0 && data[i+2] == 3) {
            out.push_back(0);
            out.push_back(0);
            i += 2; // skip emulation prevention byte
        } else {
            out.push_back(data[i]);
        }
    }
    return out;
}

enum NaluType : uint8_t {
    kSlice = 1,
    kIdr = 5,
    kSei = 6,
    kSps = 7,
    kPps = 8,
    kAud = 9,
    kEndOfSequence = 10,
    kEndOfStream = 11,
    kFiller = 12,
    kStapA = 24,
    kFuA = 28,
};

} // namespace H264
} // namespace webrtc
