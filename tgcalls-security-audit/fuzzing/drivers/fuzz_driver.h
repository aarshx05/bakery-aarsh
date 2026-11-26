// Generic fuzz driver that reads test inputs from files or generates them.
// Usage: ./harness [file1 file2 ...] or ./harness --generate N
#ifndef FUZZ_DRIVER_H
#define FUZZ_DRIVER_H

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <filesystem>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

namespace fuzz {

inline std::vector<uint8_t> readFile(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());
}

inline std::vector<uint8_t> generateRandom(std::mt19937 &rng, size_t maxSize) {
    std::uniform_int_distribution<size_t> sizeDist(0, maxSize);
    std::uniform_int_distribution<int> byteDist(0, 255);
    size_t sz = sizeDist(rng);
    std::vector<uint8_t> data(sz);
    for (auto &b : data) b = static_cast<uint8_t>(byteDist(rng));
    return data;
}

// Mutate an existing input
inline std::vector<uint8_t> mutate(std::mt19937 &rng, const std::vector<uint8_t> &base) {
    if (base.empty()) return generateRandom(rng, 256);
    std::vector<uint8_t> result = base;
    std::uniform_int_distribution<int> opDist(0, 5);
    std::uniform_int_distribution<int> byteDist(0, 255);

    int ops = 1 + (opDist(rng) % 3);
    for (int i = 0; i < ops; i++) {
        int op = opDist(rng);
        if (result.empty()) { result.push_back(byteDist(rng)); continue; }
        std::uniform_int_distribution<size_t> posDist(0, result.size() - 1);
        size_t pos = posDist(rng);

        switch (op) {
            case 0: // flip byte
                result[pos] = byteDist(rng);
                break;
            case 1: // insert byte
                result.insert(result.begin() + pos, byteDist(rng));
                break;
            case 2: // delete byte
                result.erase(result.begin() + pos);
                break;
            case 3: // flip bit
                result[pos] ^= (1 << (rng() % 8));
                break;
            case 4: // set to boundary value
                { uint8_t boundaries[] = {0, 1, 0x7f, 0x80, 0xfe, 0xff};
                  result[pos] = boundaries[rng() % 6]; }
                break;
            case 5: // duplicate a chunk
                if (result.size() < 4096) {
                    size_t len = std::min(size_t(16), result.size() - pos);
                    result.insert(result.begin() + pos, result.begin() + pos, result.begin() + pos + len);
                }
                break;
        }
    }
    return result;
}

inline int runDriver(int argc, char **argv, size_t maxInputSize = 131072) {
    // If files are provided, run on those
    if (argc > 1 && std::string(argv[1]) != "--generate") {
        int crashes = 0;
        for (int i = 1; i < argc; i++) {
            auto data = readFile(argv[i]);
            if (data.empty()) continue;
            LLVMFuzzerTestOneInput(data.data(), data.size());
        }
        return 0;
    }

    // Otherwise, generate and mutate
    int iterations = 50000;
    if (argc > 2) iterations = std::atoi(argv[2]);

    std::mt19937 rng(42); // deterministic seed for reproducibility
    std::vector<std::vector<uint8_t>> corpus;

    // Seed corpus with some interesting inputs
    corpus.push_back({}); // empty
    corpus.push_back({0}); // single byte
    corpus.push_back({0, 0, 0, 0}); // 4 zero bytes
    corpus.push_back({0xff, 0xff, 0xff, 0xff}); // 4 max bytes
    // 21 bytes (minimum EncryptedConnection packet)
    corpus.push_back(std::vector<uint8_t>(21, 0));
    // random seeds
    for (int i = 0; i < 20; i++) {
        corpus.push_back(generateRandom(rng, maxInputSize));
    }

    std::cerr << "[fuzz] Running " << iterations << " iterations with ASAN+UBSAN\n";
    for (int i = 0; i < iterations; i++) {
        // Pick a base from corpus and mutate
        auto &base = corpus[rng() % corpus.size()];
        auto input = mutate(rng, base);

        LLVMFuzzerTestOneInput(input.data(), input.size());

        // Occasionally add to corpus
        if ((rng() % 100) < 5 && corpus.size() < 1000) {
            corpus.push_back(input);
        }

        if ((i + 1) % 10000 == 0) {
            std::cerr << "[fuzz] " << (i + 1) << "/" << iterations
                      << " iterations, corpus size: " << corpus.size() << "\n";
        }
    }
    std::cerr << "[fuzz] Done. " << iterations << " iterations completed.\n";
    return 0;
}

} // namespace fuzz

#endif // FUZZ_DRIVER_H
