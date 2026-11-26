// Standalone fuzz harness for tgcalls signaling JSON parsing
// Tests json11 parsing with the patterns used in Signaling.cpp
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
// Using our standard driver

// Include json11 directly
#include "third-party/json11.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) return 0;

    std::string input(reinterpret_cast<const char*>(data), size);
    std::string err;

    // Test json11 parsing (same as Signaling.cpp does)
    auto json = json11::Json::parse(input, err);
    if (!json.is_null() && json.is_object()) {
        // Exercise the field access patterns used in Signaling.cpp
        auto type = json["@type"].string_value();

        if (type == "InitialSetup") {
            json["ufrag"].string_value();
            json["pwd"].string_value();
            auto fps = json["fingerprints"];
            if (fps.is_array()) {
                for (auto &fp : fps.array_items()) {
                    fp["hash"].string_value();
                    fp["setup"].string_value();
                    fp["fingerprint"].string_value();
                }
            }
        } else if (type == "Candidates") {
            auto candidates = json["candidates"];
            if (candidates.is_array()) {
                for (auto &c : candidates.array_items()) {
                    c["sdpString"].string_value();
                    c["port"].int_value();
                    c["protocol"].string_value();
                }
            }
        } else if (type == "NegotiateChannels") {
            json["exchangeId"].string_value();
            auto contents = json["contents"];
            if (contents.is_array()) {
                for (auto &content : contents.array_items()) {
                    content["type"].string_value();
                    content["ssrc"].string_value();
                    auto payloadTypes = content["payloadTypes"];
                    if (payloadTypes.is_array()) {
                        for (auto &pt : payloadTypes.array_items()) {
                            pt["id"].int_value();
                            pt["name"].string_value();
                            pt["clockrate"].int_value();
                            pt["channels"].int_value();
                            auto fbTypes = pt["feedbackTypes"];
                            if (fbTypes.is_array()) {
                                for (auto &fb : fbTypes.array_items()) {
                                    fb["type"].string_value();
                                    fb["subtype"].string_value();
                                }
                            }
                            auto params = pt["parameters"];
                            if (params.is_object()) {
                                for (auto &p : params.object_items()) {
                                    (void)p.first;
                                    p.second.string_value();
                                }
                            }
                        }
                    }
                    auto rtpExtensions = content["rtpExtensions"];
                    if (rtpExtensions.is_array()) {
                        for (auto &ext : rtpExtensions.array_items()) {
                            ext["id"].int_value();
                            ext["uri"].string_value();
                        }
                    }
                }
            }
        } else if (type == "MediaState") {
            json["muted"].bool_value();
            json["videoState"].string_value();
            json["videoRotation"].int_value();
            json["screencastState"].string_value();
            json["lowBattery"].bool_value();
        }
    }

    return 0;
}

#include "driver.h"
