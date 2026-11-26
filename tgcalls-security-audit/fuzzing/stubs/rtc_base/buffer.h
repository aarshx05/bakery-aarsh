#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <algorithm>

namespace webrtc {

class Buffer {
public:
    Buffer() = default;
    explicit Buffer(size_t size) : data_(size) {}
    Buffer(const uint8_t* data, size_t size) : data_(data, data + size) {}

    uint8_t* data() { return data_.data(); }
    const uint8_t* data() const { return data_.data(); }
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

    void SetSize(size_t size) { data_.resize(size); }
    void AppendData(const uint8_t* data, size_t size) {
        data_.insert(data_.end(), data, data + size);
    }

private:
    std::vector<uint8_t> data_;
};

} // namespace webrtc
