// Targeted test for ReadSeq() unaligned access UB
// From EncryptedConnection.cpp line 48-50:
//   auto ReadSeq(const char *data) {
//       return *reinterpret_cast<const uint32_t*>(data);
//   }
// This is UB when data is not 4-byte aligned.
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iostream>

// Reproduce the exact code from EncryptedConnection.cpp
auto ReadSeq(const char *data) {
    return *reinterpret_cast<const uint32_t*>(data);
}

int main() {
    // Test 1: aligned access (should be fine)
    alignas(4) uint8_t aligned_buf[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint32_t val1 = ReadSeq(reinterpret_cast<const char*>(aligned_buf));
    std::cerr << "Aligned read: 0x" << std::hex << val1 << "\n";

    // Test 2: unaligned by 1 byte (triggers UB)
    uint8_t unaligned_buf[9] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint32_t val2 = ReadSeq(reinterpret_cast<const char*>(unaligned_buf + 1));
    std::cerr << "Unaligned+1 read: 0x" << std::hex << val2 << "\n";

    // Test 3: unaligned by 2 bytes
    uint32_t val3 = ReadSeq(reinterpret_cast<const char*>(unaligned_buf + 2));
    std::cerr << "Unaligned+2 read: 0x" << std::hex << val3 << "\n";

    // Test 4: unaligned by 3 bytes
    uint32_t val4 = ReadSeq(reinterpret_cast<const char*>(unaligned_buf + 3));
    std::cerr << "Unaligned+3 read: 0x" << std::hex << val4 << "\n";

    std::cerr << "All reads completed (UBSAN should flag unaligned ones)\n";
    return 0;
}
