#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

static std::vector<uint8_t> parseHex(std::string const &hex) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        uint8_t high = 0, low = 0;
        char c = hex[i];
        if (c >= '0' && c <= '9') high = c - '0';
        else if (c >= 'a' && c <= 'f') high = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') high = c - 'A' + 10;
        c = hex[i + 1];
        if (c >= '0' && c <= '9') low = c - '0';
        else if (c >= 'a' && c <= 'f') low = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') low = c - 'A' + 10;
        result.push_back((high << 4) | low);
    }
    return result;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;
    uint8_t selector = data[0];
    data++; size--;

    switch (selector & 0x01) {
        case 0: {
            std::string hexStr(reinterpret_cast<const char *>(data), size);
            auto result = parseHex(hexStr);
            volatile size_t s = result.size();
            (void)s;
            break;
        }
        case 1: {
            // parseHex with odd-length strings, NUL bytes, etc.
            std::string hexStr(reinterpret_cast<const char *>(data), size);
            if (!hexStr.empty()) {
                auto result = parseHex(hexStr.substr(0, hexStr.size() / 2));
                volatile size_t s = result.size();
                (void)s;
            }
            break;
        }
    }
    return 0;
}

#include "driver.h"
