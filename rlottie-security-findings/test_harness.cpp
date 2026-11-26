#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include "rlottie.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <lottie.json> [width] [height]\n", argv[0]);
        return 1;
    }

    std::ifstream ifs(argv[1]);
    if (!ifs.good()) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return 1;
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string json = ss.str();

    size_t overrideW = (argc > 2) ? (size_t)atoi(argv[2]) : 0;
    size_t overrideH = (argc > 3) ? (size_t)atoi(argv[3]) : 0;

    auto animation = rlottie::Animation::loadFromData(
        std::move(json), "test_key", "", false);

    if (!animation) {
        fprintf(stderr, "Failed to parse animation\n");
        return 0;
    }

    size_t w = 0, h = 0;
    animation->size(w, h);
    fprintf(stderr, "Animation size: %zux%zu, frames: %zu, fps: %.1f\n",
            w, h, animation->totalFrame(), animation->frameRate());

    size_t renderW = overrideW ? overrideW : (w > 0 ? (w > 1024 ? 1024 : w) : 100);
    size_t renderH = overrideH ? overrideH : (h > 0 ? (h > 1024 ? 1024 : h) : 100);
    size_t stride = renderW * 4;

    std::vector<uint32_t> buffer(renderW * renderH);

    auto renderFrame = [&](size_t frameNo) {
        fprintf(stderr, "Rendering frame %zu at %zux%zu...\n", frameNo, renderW, renderH);
        memset(buffer.data(), 0, buffer.size() * sizeof(uint32_t));
        rlottie::Surface surface(buffer.data(), renderW, renderH, stride);
        animation->renderSync(frameNo, surface);
        fprintf(stderr, "  Frame %zu done\n", frameNo);
    };

    size_t totalFrames = animation->totalFrame();

    renderFrame(0);

    if (totalFrames > 2) {
        renderFrame(totalFrames / 2);
    }

    if (totalFrames > 1) {
        renderFrame(totalFrames - 1);
    }

    renderFrame(totalFrames + 1);

    // small surface
    {
        std::vector<uint32_t> smallBuf(2 * 2);
        rlottie::Surface smallSurf(smallBuf.data(), 2, 2, 2 * 4);
        animation->renderSync(0, smallSurf);
    }

    // large surface
    {
        size_t bigW = 2048, bigH = 2048;
        std::vector<uint32_t> bigBuf(bigW * bigH);
        rlottie::Surface bigSurf(bigBuf.data(), bigW, bigH, bigW * 4);
        animation->renderSync(0, bigSurf);
    }

    fprintf(stderr, "All renders completed successfully\n");
    return 0;
}
