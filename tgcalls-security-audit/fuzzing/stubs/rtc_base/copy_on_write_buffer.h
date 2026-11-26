#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include "api/array_view.h"

namespace webrtc {

class CopyOnWriteBuffer {
public:
    CopyOnWriteBuffer() = default;

    explicit CopyOnWriteBuffer(size_t size) : data_(size, 0) {}

    CopyOnWriteBuffer(const uint8_t* data, size_t size)
        : data_(data, data + size) {}

    const uint8_t* data() const { return data_.data(); }
    const uint8_t* cdata() const { return data_.data(); }
    uint8_t* MutableData() { return data_.data(); }
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

    void SetSize(size_t size) { data_.resize(size); }

    void AppendData(const uint8_t* data, size_t size) {
        data_.insert(data_.end(), data, data + size);
    }
    void AppendData(const char* data, size_t size) {
        AppendData(reinterpret_cast<const uint8_t*>(data), size);
    }
    void AppendData(const CopyOnWriteBuffer& other) {
        data_.insert(data_.end(), other.data_.begin(), other.data_.end());
    }

    const uint8_t* begin() const { return data_.data(); }
    const uint8_t* end() const { return data_.data() + data_.size(); }

    bool operator==(const CopyOnWriteBuffer& o) const { return data_ == o.data_; }
    bool operator!=(const CopyOnWriteBuffer& o) const { return data_ != o.data_; }

private:
    std::vector<uint8_t> data_;
};

} // namespace webrtc
