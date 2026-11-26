#pragma once
#include <cstddef>
#include <type_traits>

namespace webrtc {

template <typename T>
class ArrayView {
public:
    ArrayView() : data_(nullptr), size_(0) {}
    ArrayView(T* data, size_t size) : data_(data), size_(size) {}

    template <typename U, typename = std::enable_if_t<
        std::is_convertible_v<U*, T*>>>
    ArrayView(const ArrayView<U>& other)
        : data_(other.data()), size_(other.size()) {}

    // Construct from contiguous container-like things
    template <typename C,
              typename = std::enable_if_t<!std::is_array_v<std::remove_reference_t<C>>>,
              typename = decltype(std::declval<C>().data()),
              typename = decltype(std::declval<C>().size())>
    ArrayView(C& c) : data_(c.data()), size_(c.size()) {}

    T* data() const { return data_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    T& operator[](size_t i) const { return data_[i]; }

    T* begin() const { return data_; }
    T* end() const { return data_ + size_; }

private:
    T* data_;
    size_t size_;
};

} // namespace webrtc
