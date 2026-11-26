#pragma once
#include <cstdint>
#include <cstddef>
#include <climits>
#include "api/array_view.h"

namespace webrtc {

class BitstreamReader {
public:
    BitstreamReader(ArrayView<const uint8_t> bytes)
        : data_(bytes.data()), size_(bytes.size()), bit_pos_(0), ok_(true) {}

    BitstreamReader(const uint8_t* data, size_t size)
        : data_(data), size_(size), bit_pos_(0), ok_(true) {}

    uint32_t ReadBits(int num_bits) {
        if (num_bits <= 0 || num_bits > 32) { ok_ = false; return 0; }
        uint32_t result = 0;
        for (int i = 0; i < num_bits; ++i) {
            size_t byte_idx = bit_pos_ / 8;
            if (byte_idx >= size_) { ok_ = false; return 0; }
            int bit_idx = 7 - (bit_pos_ % 8);
            result = (result << 1) | ((data_[byte_idx] >> bit_idx) & 1);
            ++bit_pos_;
        }
        return result;
    }

    uint32_t ReadBit() { return ReadBits(1); }

    uint32_t ReadExponentialGolomb() {
        int leading_zeros = 0;
        while (ReadBit() == 0) {
            ++leading_zeros;
            if (leading_zeros > 31 || !ok_) { ok_ = false; return 0; }
        }
        if (leading_zeros == 0) return 0;
        uint32_t rest = ReadBits(leading_zeros);
        if (!ok_) return 0;
        return (1u << leading_zeros) - 1 + rest;
    }

    void ConsumeBits(int num_bits) {
        for (int i = 0; i < num_bits; ++i) {
            if (bit_pos_ / 8 >= size_) { ok_ = false; return; }
            ++bit_pos_;
        }
    }

    bool Ok() const { return ok_; }
    int RemainingBitCount() const {
        return ok_ ? static_cast<int>(size_ * 8 - bit_pos_) : 0;
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t bit_pos_;
    bool ok_;
};

} // namespace webrtc
