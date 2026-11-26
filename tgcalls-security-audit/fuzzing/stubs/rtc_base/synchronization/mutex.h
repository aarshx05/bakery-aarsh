#pragma once
#include <mutex>

namespace webrtc {

class Mutex {
public:
    void Lock() { m_.lock(); }
    void Unlock() { m_.unlock(); }
private:
    std::mutex m_;
};

class MutexLock {
public:
    explicit MutexLock(Mutex* m) : m_(m) { m_->Lock(); }
    ~MutexLock() { m_->Unlock(); }
private:
    Mutex* m_;
};

} // namespace webrtc
