#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "api/array_view.h"

namespace webrtc {

template <typename T>
class scoped_refptr {
public:
    scoped_refptr() : ptr_(nullptr) {}
    scoped_refptr(T* p) : ptr_(p) {}
    scoped_refptr(const scoped_refptr& o) : ptr_(o.ptr_) {}
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    operator bool() const { return ptr_ != nullptr; }
private:
    T* ptr_;
};

class TransformableFrameInterface {
public:
    virtual ~TransformableFrameInterface() = default;
    virtual ArrayView<const uint8_t> GetData() const = 0;
    virtual void SetData(ArrayView<const uint8_t> data) = 0;
    virtual uint32_t GetSsrc() const = 0;
    virtual uint8_t GetPayloadType() const = 0;
};

class TransformedFrameCallback {
public:
    virtual ~TransformedFrameCallback() = default;
    virtual void OnTransformedFrame(std::unique_ptr<TransformableFrameInterface> frame) = 0;
};

class FrameTransformerInterface {
public:
    virtual ~FrameTransformerInterface() = default;
    virtual void RegisterTransformedFrameCallback(
        scoped_refptr<TransformedFrameCallback> callback) = 0;
    virtual void RegisterTransformedFrameSinkCallback(
        scoped_refptr<TransformedFrameCallback> callback, uint32_t ssrc) = 0;
    virtual void UnregisterTransformedFrameSinkCallback(uint32_t ssrc) = 0;
    virtual void Transform(std::unique_ptr<TransformableFrameInterface> frame) = 0;
};

} // namespace webrtc
