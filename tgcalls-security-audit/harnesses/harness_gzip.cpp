// Fuzz harness for tgcalls gzip decompression
// Tests gunzipData() and gzipData() with ASAN+UBSAN
#include "drivers/fuzz_driver.h"
#include "utils/gzip.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    std::vector<uint8_t> input(data, data + size);

    tgcalls::isGzip(input);
    tgcalls::gunzipData(input, 1024);
    tgcalls::gunzipData(input, 1024 * 1024);
    tgcalls::gunzipData(input, 2 * 1024 * 1024);

    if (size < 4096) {
        auto compressed = tgcalls::gzipData(input);
        if (compressed.has_value()) {
            tgcalls::gunzipData(*compressed, size + 1024);
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    return fuzz::runDriver(argc, argv, 65536);
}
