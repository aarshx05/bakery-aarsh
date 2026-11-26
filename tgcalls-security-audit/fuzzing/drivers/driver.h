#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input-file> [input-file2 ...]\n", argv[0]);
        fprintf(stderr, "  Reads each file and feeds to harness under ASAN+UBSAN.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "rb");
        if (!f) {
            fprintf(stderr, "[!] Cannot open: %s\n", argv[i]);
            continue;
        }
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (fsize < 0 || fsize > 100 * 1024 * 1024) {
            fprintf(stderr, "[!] Skipping %s (size=%ld)\n", argv[i], fsize);
            fclose(f);
            continue;
        }

        std::vector<uint8_t> buf(fsize);
        if (fsize > 0) {
            size_t nread = fread(buf.data(), 1, fsize, f);
            buf.resize(nread);
        }
        fclose(f);

        fprintf(stderr, "[*] Running %s (%zu bytes)...\n", argv[i], buf.size());
        LLVMFuzzerTestOneInput(buf.data(), buf.size());
        fprintf(stderr, "[+] OK: %s\n", argv[i]);
    }

    fprintf(stderr, "[*] All %d inputs processed.\n", argc - 1);
    return 0;
}
