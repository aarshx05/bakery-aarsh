// libFuzzer harness for gzip decompression
// Compile: clang++ -std=c++17 -fsanitize=address,undefined,fuzzer \
//   -I<tgcalls_src> \
//   fuzz_gzip.cpp utils/gzip.cpp \
//   -lz -o fuzz_gzip
//
// Exercises: gunzipData with size limit (zip bomb protection)
// Attack model: peer sends crafted compressed signaling data.

#include "utils/gzip.h"
#include <cstdint>
#include <cstddef>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) return 0;

    std::vector<uint8_t> input(data, data + size);

    // Test with signaling size limit (2MB, same as kV2DecompressSizeLimit)
    auto result = tgcalls::gunzipData(input, 2 * 1024 * 1024);

    // Also test with zero limit (no limit)
    auto result2 = tgcalls::gunzipData(input, 0);

    // Test isGzip detection
    volatile bool isGz = tgcalls::isGzip(input);
    (void)isGz;

    return 0;
}
