#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>
#include "api/array_view.h"
#include "rtc_base/copy_on_write_buffer.h"

namespace webrtc {

class ByteBufferWriter {
public:
    ByteBufferWriter() = default;

    void WriteUInt8(uint8_t val) {
        buf_.push_back(val);
    }
    void WriteUInt16(uint16_t val) {
        buf_.push_back(static_cast<uint8_t>(val >> 8));
        buf_.push_back(static_cast<uint8_t>(val));
    }
    void WriteUInt32(uint32_t val) {
        buf_.push_back(static_cast<uint8_t>(val >> 24));
        buf_.push_back(static_cast<uint8_t>(val >> 16));
        buf_.push_back(static_cast<uint8_t>(val >> 8));
        buf_.push_back(static_cast<uint8_t>(val));
    }
    void Write(const uint8_t* data, size_t len) {
        buf_.insert(buf_.end(), data, data + len);
    }

    template<typename T>
    void Write(T span_like) {
        auto* d = reinterpret_cast<const uint8_t*>(span_like.data());
        buf_.insert(buf_.end(), d, d + span_like.size());
    }

    void WriteString(const std::string& s) {
        buf_.insert(buf_.end(), s.begin(), s.end());
    }

    const char* Data() const { return reinterpret_cast<const char*>(buf_.data()); }
    size_t Length() const { return buf_.size(); }

private:
    std::vector<uint8_t> buf_;
};

class ByteBufferReader {
public:
    ByteBufferReader(ArrayView<const uint8_t> view)
        : data_(reinterpret_cast<const char*>(view.data())), size_(view.size()), pos_(0) {}

    ByteBufferReader(const char* data, size_t size)
        : data_(data), size_(size), pos_(0) {}

    // Copy-construct (snapshot of current position)
    ByteBufferReader(const ByteBufferReader&) = default;

    bool ReadUInt8(uint8_t* val) {
        if (remaining() < 1) return false;
        *val = static_cast<uint8_t>(data_[pos_++]);
        return true;
    }

    bool ReadUInt16(uint16_t* val) {
        if (remaining() < 2) return false;
        const uint8_t* p = cur();
        *val = static_cast<uint16_t>((uint16_t(p[0]) << 8) | p[1]);
        pos_ += 2;
        return true;
    }

    bool ReadUInt32(uint32_t* val) {
        if (remaining() < 4) return false;
        const uint8_t* p = cur();
        *val = (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16)
             | (uint32_t(p[2]) << 8)  | uint32_t(p[3]);
        pos_ += 4;
        return true;
    }

    bool ReadBytes(ArrayView<uint8_t> dst) {
        if (remaining() < dst.size()) return false;
        std::memcpy(dst.data(), data_ + pos_, dst.size());
        pos_ += dst.size();
        return true;
    }

    bool ReadString(std::string* val, size_t len) {
        if (remaining() < len) return false;
        val->assign(data_ + pos_, len);
        pos_ += len;
        return true;
    }

    CopyOnWriteBuffer ReadBuffer(size_t len) {
        size_t actual = std::min(len, remaining());
        CopyOnWriteBuffer buf(cur(), actual);
        pos_ += actual;
        return buf;
    }

    void Consume(size_t len) {
        size_t actual = std::min(len, remaining());
        pos_ += actual;
    }

    const char* Data() const { return data_ + pos_; }
    size_t Length() const { return remaining(); }
    bool Ok() const { return true; } // simplified

private:
    const uint8_t* cur() const {
        return reinterpret_cast<const uint8_t*>(data_ + pos_);
    }
    size_t remaining() const { return (pos_ <= size_) ? (size_ - pos_) : 0; }

    const char* data_;
    size_t size_;
    size_t pos_;
};

} // namespace webrtc
