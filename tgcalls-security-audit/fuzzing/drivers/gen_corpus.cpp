// Generate seed corpus files for each fuzzing target
// Compile: g++ -std=c++17 -o gen_corpus gen_corpus.cpp
// Usage: ./gen_corpus <output_dir>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <random>

static void writeFile(const std::string &path, const std::vector<uint8_t> &data) {
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char *>(data.data()), data.size());
}

static void put32be(std::vector<uint8_t> &buf, uint32_t val) {
    buf.push_back((val >> 24) & 0xff);
    buf.push_back((val >> 16) & 0xff);
    buf.push_back((val >> 8) & 0xff);
    buf.push_back(val & 0xff);
}

static void put16be(std::vector<uint8_t> &buf, uint16_t val) {
    buf.push_back((val >> 8) & 0xff);
    buf.push_back(val & 0xff);
}

// Signaling corpus: JSON messages matching tgcalls signaling format
static void genSignalingCorpus(const std::string &dir) {
    // InitialSetup message
    writeFile(dir + "/sig_initial_setup.bin", std::vector<uint8_t>(
        (uint8_t *)R"({"@type":"InitialSetup","ufrag":"abc","pwd":"def","fingerprints":[{"hash":"sha-256","setup":"actpass","fingerprint":"AA:BB"}]})",
        (uint8_t *)R"({"@type":"InitialSetup","ufrag":"abc","pwd":"def","fingerprints":[{"hash":"sha-256","setup":"actpass","fingerprint":"AA:BB"}]})" + 120));

    // Candidates message
    writeFile(dir + "/sig_candidates.bin", std::vector<uint8_t>(
        (uint8_t *)R"({"@type":"Candidates","candidates":[{"sdpString":"candidate:1 1 udp 2130706431 192.168.1.1 50000 typ host"}]})",
        (uint8_t *)R"({"@type":"Candidates","candidates":[{"sdpString":"candidate:1 1 udp 2130706431 192.168.1.1 50000 typ host"}]})" + 108));

    // MediaState message
    writeFile(dir + "/sig_media_state.bin", std::vector<uint8_t>(
        (uint8_t *)R"({"@type":"MediaState","muted":false,"videoState":"active","videoRotation":0,"screencastState":"inactive","lowBattery":false})",
        (uint8_t *)R"({"@type":"MediaState","muted":false,"videoState":"active","videoRotation":0,"screencastState":"inactive","lowBattery":false})" + 123));

    // NegotiateChannels message (v2 only)
    writeFile(dir + "/sig_negotiate.bin", std::vector<uint8_t>(
        (uint8_t *)R"({"@type":"NegotiateChannels","exchangeId":"1","contents":[{"type":"audio","ssrc":"12345","payloadTypes":[{"id":111,"name":"opus","clockrate":48000,"channels":2}],"rtpExtensions":[]}]})",
        (uint8_t *)R"({"@type":"NegotiateChannels","exchangeId":"1","contents":[{"type":"audio","ssrc":"12345","payloadTypes":[{"id":111,"name":"opus","clockrate":48000,"channels":2}],"rtpExtensions":[]}]})" + 189));

    // Malformed JSON
    writeFile(dir + "/sig_malformed1.bin", std::vector<uint8_t>({'{', '"', '@', 't'}));
    writeFile(dir + "/sig_malformed2.bin", std::vector<uint8_t>(
        (uint8_t *)R"({"@type":"InitialSetup"})",
        (uint8_t *)R"({"@type":"InitialSetup"})" + 24));

    // Huge nested JSON
    std::string huge = R"({"@type":"NegotiateChannels","exchangeId":"1","contents":[)";
    for (int i = 0; i < 100; i++) {
        if (i > 0) huge += ",";
        huge += R"({"type":"audio","ssrc":")" + std::to_string(i) + R"(","payloadTypes":[],"rtpExtensions":[]})";
    }
    huge += "]}";
    writeFile(dir + "/sig_huge.bin", std::vector<uint8_t>(huge.begin(), huge.end()));

    std::cerr << "  Generated signaling corpus\n";
}

// Message deserialization corpus: binary TLV format
static void genMessageCorpus(const std::string &dir) {
    // Message type 1 (CandidatesListMessage) with minimal data
    {
        std::vector<uint8_t> msg;
        msg.push_back(1); // type = CandidatesListMessage
        put16be(msg, 10); // length (for non-single-message)
        // Some candidate data
        for (int i = 0; i < 10; i++) msg.push_back(0x41 + i);
        writeFile(dir + "/msg_candidates.bin", msg);
    }
    // Message type 5 (AudioDataMessage) with buffer
    {
        std::vector<uint8_t> msg;
        msg.push_back(5); // type = AudioDataMessage
        put16be(msg, 160); // length
        for (int i = 0; i < 160; i++) msg.push_back(i & 0xff);
        writeFile(dir + "/msg_audio.bin", msg);
    }
    // Raw message (uint32 length + data)
    {
        std::vector<uint8_t> msg;
        put32be(msg, 100); // length
        for (int i = 0; i < 100; i++) msg.push_back(i & 0xff);
        writeFile(dir + "/msg_raw.bin", msg);
    }
    // Raw message with oversized length (should be rejected)
    {
        std::vector<uint8_t> msg;
        put32be(msg, 0x00200000); // 2MB > 1MB limit
        for (int i = 0; i < 20; i++) msg.push_back(0);
        writeFile(dir + "/msg_raw_oversize.bin", msg);
    }
    // Edge: length exactly at 1MB boundary
    {
        std::vector<uint8_t> msg;
        put32be(msg, 1024 * 1024); // exactly 1MB
        for (int i = 0; i < 20; i++) msg.push_back(0);
        writeFile(dir + "/msg_raw_1mb.bin", msg);
    }
    // Zero-length raw message
    {
        std::vector<uint8_t> msg;
        put32be(msg, 0);
        writeFile(dir + "/msg_raw_zero.bin", msg);
    }
    // Invalid type byte
    {
        std::vector<uint8_t> msg = {0xff, 0, 0, 0, 0};
        writeFile(dir + "/msg_invalid_type.bin", msg);
    }

    std::cerr << "  Generated message corpus\n";
}

// H264 frame corpus for GroupFrameTransformer
static void genH264Corpus(const std::string &dir) {
    // Simple IDR slice with 4-byte start code
    {
        std::vector<uint8_t> frame = {0, 0, 0, 1, 0x65}; // 00 00 00 01 + IDR NAL
        for (int i = 0; i < 100; i++) frame.push_back(i & 0xff);
        writeFile(dir + "/h264_idr.bin", frame);
    }
    // SPS + PPS + IDR (typical keyframe)
    {
        std::vector<uint8_t> frame;
        // SPS
        frame.insert(frame.end(), {0, 0, 0, 1, 0x67}); // SPS NAL
        for (int i = 0; i < 20; i++) frame.push_back(0x42 + i);
        // PPS
        frame.insert(frame.end(), {0, 0, 0, 1, 0x68}); // PPS NAL
        for (int i = 0; i < 10; i++) frame.push_back(0x11 + i);
        // IDR
        frame.insert(frame.end(), {0, 0, 0, 1, 0x65}); // IDR NAL
        for (int i = 0; i < 200; i++) frame.push_back(i & 0xff);
        writeFile(dir + "/h264_keyframe.bin", frame);
    }
    // 3-byte start codes (triggers start code expansion)
    {
        std::vector<uint8_t> frame;
        frame.insert(frame.end(), {0, 0, 1, 0x67}); // 3-byte SPS
        for (int i = 0; i < 15; i++) frame.push_back(0x42);
        frame.insert(frame.end(), {0, 0, 1, 0x68}); // 3-byte PPS
        for (int i = 0; i < 8; i++) frame.push_back(0x33);
        frame.insert(frame.end(), {0, 0, 1, 0x65}); // 3-byte IDR
        for (int i = 0; i < 100; i++) frame.push_back(i);
        writeFile(dir + "/h264_3byte_startcodes.bin", frame);
    }
    // FU-A fragmented NAL
    {
        std::vector<uint8_t> frame = {0, 0, 0, 1, 0x1c}; // FU-A indicator
        frame.push_back(0x85); // FU header: start=1, type=5 (IDR)
        for (int i = 0; i < 50; i++) frame.push_back(i);
        writeFile(dir + "/h264_fua.bin", frame);
    }
    // STAP-A aggregation packet
    {
        std::vector<uint8_t> frame = {0, 0, 0, 1, 0x18}; // STAP-A
        put16be(frame, 5); // length of first NAL
        frame.push_back(0x67); // SPS
        for (int i = 0; i < 4; i++) frame.push_back(0x42);
        put16be(frame, 3); // length of second NAL
        frame.push_back(0x68); // PPS
        frame.push_back(0x11); frame.push_back(0x22);
        writeFile(dir + "/h264_stapa.bin", frame);
    }
    // Edge: frame with embedded fake start codes in data
    {
        std::vector<uint8_t> frame = {0, 0, 0, 1, 0x65};
        frame.push_back(0x00); frame.push_back(0x00); frame.push_back(0x01); // fake start code
        for (int i = 0; i < 50; i++) frame.push_back(i);
        writeFile(dir + "/h264_embedded_startcode.bin", frame);
    }
    // Empty frame
    writeFile(dir + "/h264_empty.bin", {});
    // Single byte
    writeFile(dir + "/h264_1byte.bin", {0x65});

    std::cerr << "  Generated H264 corpus\n";
}

// Gzip corpus
static void genGzipCorpus(const std::string &dir) {
    // Valid gzip header with minimal data
    writeFile(dir + "/gz_valid_header.bin", {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03});
    // Zlib header
    writeFile(dir + "/gz_zlib_header.bin", {0x78, 0x9c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01});
    // Not gzip
    writeFile(dir + "/gz_not_gzip.bin", {0x41, 0x42, 0x43});
    // Truncated gzip
    writeFile(dir + "/gz_truncated.bin", {0x1f, 0x8b});
    // Large gzip with high compression ratio (near zip bomb)
    // A stream of zeros compresses very well
    {
        std::vector<uint8_t> data = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
        // Add some deflate data (not valid, just for fuzzing)
        for (int i = 0; i < 100; i++) data.push_back(0x00);
        writeFile(dir + "/gz_potential_bomb.bin", data);
    }

    std::cerr << "  Generated gzip corpus\n";
}

int main(int argc, char **argv) {
    std::string outDir = argc > 1 ? argv[1] : "/home/user/tgcalls-audit/fuzz-build/corpus";

    std::filesystem::create_directories(outDir + "/signaling");
    std::filesystem::create_directories(outDir + "/message");
    std::filesystem::create_directories(outDir + "/h264");
    std::filesystem::create_directories(outDir + "/gzip");

    std::cerr << "Generating seed corpus in " << outDir << "\n";
    genSignalingCorpus(outDir + "/signaling");
    genMessageCorpus(outDir + "/message");
    genH264Corpus(outDir + "/h264");
    genGzipCorpus(outDir + "/gzip");
    std::cerr << "Done.\n";
    return 0;
}
