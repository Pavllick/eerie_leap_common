#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <utility>

#include "subsys/cdmp/types/cdmp_types.h"

namespace eerie_leap::subsys::cdmp::models {

using namespace eerie_leap::subsys::cdmp::types;

// (Base + 3)
struct CdmpCommandResponseMessage {
    uint8_t source_device_id;
    CdmpCommandCode command_code;
    uint8_t transaction_id;
    CdmpResultCode result_code;
    std::vector<uint8_t> data;

    static CdmpCommandResponseMessage FromCanFrame(std::span<const uint8_t> frame_data) {
        CdmpCommandResponseMessage message = {};
        message.source_device_id = frame_data[0];
        message.command_code = static_cast<CdmpCommandCode>(frame_data[1]);
        message.transaction_id = frame_data[2];
        message.result_code = static_cast<CdmpResultCode>(frame_data[3]);
        message.data = std::vector<uint8_t>(frame_data.begin() + 4, frame_data.end());

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            source_device_id,
            std::to_underlying(command_code),
            transaction_id,
            std::to_underlying(result_code)};
        frame_data.insert(frame_data.end(), data.begin(), data.end());

        return frame_data;
    }
};

} // namespace eerie_leap::subsys::cdmp::models
