#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <stdexcept>
#include <utility>

#include "cdmp_device.h"
#include "../types/cdmp_types.h"

namespace eerie_leap::subsys::cdmp::models {

using namespace eerie_leap::subsys::cdmp::types;

// Base message structures (Base + 0)
struct CdmpDiscoveryRequestMessage {
    static constexpr CdmpManagementMessageType message_type = CdmpManagementMessageType::DISCOVERY_REQUEST;
    uint32_t uid;

    static CdmpDiscoveryRequestMessage FromCanFrame(std::span<const uint8_t> frame_data) {
        if(frame_data[0] != static_cast<uint8_t>(CdmpManagementMessageType::DISCOVERY_REQUEST))
            throw std::invalid_argument("Incorrect message type");

        CdmpDiscoveryRequestMessage message = {};
        message.uid =
            (static_cast<uint32_t>(frame_data[4]) << 24)
            | (static_cast<uint32_t>(frame_data[3]) << 16)
            | (static_cast<uint32_t>(frame_data[2]) << 8)
            | static_cast<uint32_t>(frame_data[1]);

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            std::to_underlying(message_type),
            static_cast<uint8_t>(uid),
            static_cast<uint8_t>(uid >> 8),
            static_cast<uint8_t>(uid >> 16),
            static_cast<uint8_t>(uid >> 24)
        };

        return frame_data;
    }
};

// Base message structures (Base + 0)
struct CdmpDiscoveryResponseMessage {
    static constexpr CdmpManagementMessageType message_type = CdmpManagementMessageType::DISCOVERY_RESPONSE;
    uint8_t device_id;
    uint32_t uid;
    CdmpDeviceType device_type;

    static CdmpDiscoveryResponseMessage FromCanFrame(std::span<const uint8_t> frame_data) {
        if(frame_data[0] != static_cast<uint8_t>(CdmpManagementMessageType::DISCOVERY_RESPONSE))
            throw std::invalid_argument("Incorrect message type");

        CdmpDiscoveryResponseMessage message = {};
        message.device_id = frame_data[1];
        message.uid =
            (static_cast<uint32_t>(frame_data[5]) << 24)
            | (static_cast<uint32_t>(frame_data[4]) << 16)
            | (static_cast<uint32_t>(frame_data[3]) << 8)
            | static_cast<uint32_t>(frame_data[2]);
        message.device_type = static_cast<CdmpDeviceType>(frame_data[6]);

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            std::to_underlying(message_type),
            device_id,
            static_cast<uint8_t>(uid),
            static_cast<uint8_t>(uid >> 8),
            static_cast<uint8_t>(uid >> 16),
            static_cast<uint8_t>(uid >> 24),
            std::to_underlying(device_type)};

        return frame_data;
    }
};

// Base message structures (Base + 0)
struct CdmpIdClaimMessage {
    static constexpr CdmpManagementMessageType message_type = CdmpManagementMessageType::ID_CLAIM;
    uint8_t claiming_device_id;
    uint32_t uid;
    CdmpDeviceType device_type;
    uint8_t protocol_version;

    static CdmpIdClaimMessage FromCanFrame(std::span<const uint8_t> frame_data) {
        if(frame_data[0] != static_cast<uint8_t>(CdmpManagementMessageType::ID_CLAIM))
            throw std::invalid_argument("Incorrect message type");

        CdmpIdClaimMessage message = {};
        message.claiming_device_id = frame_data[1];
        message.uid =
            (static_cast<uint32_t>(frame_data[5]) << 24)
            | (static_cast<uint32_t>(frame_data[4]) << 16)
            | (static_cast<uint32_t>(frame_data[3]) << 8)
            | static_cast<uint32_t>(frame_data[2]);
        message.device_type = static_cast<CdmpDeviceType>(frame_data[6]);
        message.protocol_version = frame_data[7];

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            std::to_underlying(message_type),
            claiming_device_id,
            static_cast<uint8_t>(uid),
            static_cast<uint8_t>(uid >> 8),
            static_cast<uint8_t>(uid >> 16),
            static_cast<uint8_t>(uid >> 24),
            std::to_underlying(device_type),
            protocol_version};

        return frame_data;
    }
};

// Base message structures (Base + 0)
struct CdmpIdClaimResponseMessage {
    static constexpr CdmpManagementMessageType message_type = CdmpManagementMessageType::ID_CLAIM_RESPONSE;
    uint8_t responding_device_id;
    uint8_t claiming_device_id;
    uint32_t claiming_device_uid;
    CdmpIdClaimResult result;

    static CdmpIdClaimResponseMessage FromCanFrame(std::span<const uint8_t> frame_data) {
        if(frame_data[0] != static_cast<uint8_t>(CdmpManagementMessageType::ID_CLAIM_RESPONSE))
            throw std::invalid_argument("Incorrect message type");

        CdmpIdClaimResponseMessage message = {};
        message.responding_device_id = frame_data[1];
        message.claiming_device_id = frame_data[2];
        message.result = static_cast<CdmpIdClaimResult>(frame_data[3]);

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            std::to_underlying(message_type),
            responding_device_id,
            claiming_device_id,
            std::to_underlying(result)
        };

        return frame_data;
    }
};

// Base message structures (Base + 0)
struct CdmpHeartbeatMessage {
    static constexpr CdmpManagementMessageType message_type = CdmpManagementMessageType::HEARTBEAT;
    uint8_t device_id;
    CdmpHealthStatus health_status;
    uint8_t sequence_number;
    uint32_t capability_flags;

    static CdmpHeartbeatMessage FromCanFrame(std::span<const uint8_t> frame_data) {
        if(frame_data[0] != static_cast<uint8_t>(CdmpManagementMessageType::HEARTBEAT))
            throw std::invalid_argument("Incorrect message type");

        CdmpHeartbeatMessage message = {};
        message.device_id = frame_data[1];
        message.health_status = static_cast<CdmpHealthStatus>(frame_data[2]);
        message.sequence_number = frame_data[3];
        // Extract 32-bit capability flags (LSB first)
        message.capability_flags =
            (static_cast<uint32_t>(frame_data[7]) << 24)
            | (static_cast<uint32_t>(frame_data[6]) << 16)
            | (static_cast<uint32_t>(frame_data[5]) << 8)
            | static_cast<uint32_t>(frame_data[4]);

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            std::to_underlying(message_type),
            device_id,
            std::to_underlying(health_status),
            sequence_number,
            static_cast<uint8_t>(capability_flags),
            static_cast<uint8_t>(capability_flags >> 8),
            static_cast<uint8_t>(capability_flags >> 16),
            static_cast<uint8_t>(capability_flags >> 24)};

        return frame_data;
    }
};

// (Base + 2)
struct CdmpCommandMessage {
    // uint8_t source_device_id;
    uint8_t target_device_id;
    CdmpCommandCode command_code;
    uint8_t transaction_id;
    std::vector<uint8_t> data;

    static CdmpCommandMessage FromCanFrame(std::span<const uint8_t> frame_data) {
        CdmpCommandMessage message = {};
        message.target_device_id = frame_data[0];
        message.command_code = static_cast<CdmpCommandCode>(frame_data[1]);
        message.transaction_id = frame_data[2];
        message.data = std::vector<uint8_t>(frame_data.begin() + 3, frame_data.end());

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            // source_device_id,
            target_device_id,
            std::to_underlying(command_code),
            transaction_id};
        frame_data.insert(frame_data.end(), data.begin(), data.end());

        return frame_data;
    }
};

// (Base + 3)
struct CdmpCommandResponse {
    uint8_t source_device_id;
    CdmpCommandCode command_code;
    uint8_t transaction_id;
    CdmpResultCode result_code;
    std::vector<uint8_t> data;

    static CdmpCommandResponse FromCanFrame(std::span<const uint8_t> frame_data) {
        CdmpCommandResponse message = {};
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

// (Base + 4)
struct CdmpStateChangeNotification {
    uint8_t source_device_id;
    uint8_t state_version_number;
    CdmpStateType state_type;
    std::vector<uint8_t> data;

    static CdmpStateChangeNotification FromCanFrame(std::span<const uint8_t> frame_data) {
        CdmpStateChangeNotification message = {};
        message.source_device_id = frame_data[0];
        message.state_version_number = frame_data[1];
        message.state_type = static_cast<CdmpStateType>(frame_data[2]);
        message.data = std::vector<uint8_t>(frame_data.begin() + 3, frame_data.end());

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            source_device_id,
            state_version_number,
            std::to_underlying(state_type)};
        frame_data.insert(frame_data.end(), data.begin(), data.end());

        return frame_data;
    }
};

// (Base + 5)
struct CdmpStateChangeResponse {
    uint8_t source_device_id;
    uint8_t target_device_id;
    uint8_t state_version_number;
    CdmpResultCode response_code;
    std::vector<uint8_t> data;

    static CdmpStateChangeResponse FromCanFrame(std::span<const uint8_t> frame_data) {
        CdmpStateChangeResponse message = {};
        message.source_device_id = frame_data[0];
        message.target_device_id = frame_data[1];
        message.state_version_number = frame_data[2];
        message.response_code = static_cast<CdmpResultCode>(frame_data[3]);
        message.data = std::vector<uint8_t>(frame_data.begin() + 4, frame_data.end());

        return message;
    }

    std::vector<uint8_t> ToCanFrame() const {
        std::vector<uint8_t> frame_data = {
            source_device_id,
            target_device_id,
            state_version_number,
            std::to_underlying(response_code)};
        frame_data.insert(frame_data.end(), data.begin(), data.end());

        return frame_data;
    }
};

// struct CdmpIsoTpFrame {
//     CdmpIsoTpFrameType frame_type;
//     std::vector<uint8_t> data;
// };

// struct CdmpIsoTpFlowControl {
//     CdmpIsoTpFlowStatus flow_status;
//     uint8_t block_size;
//     uint8_t separation_time_min;
//     uint8_t reserved[4];
// };

// struct CdmpIsoTpTransferHeader {
//     uint8_t source_device_id;
//     uint8_t target_device_id;
//     CdmpIsoTpTransferType transfer_type;
//     uint8_t transaction_id;
// };

// struct CdmpBulkTransferAck {
//     uint8_t target_device_id;
//     CdmpResultCode response_code; // Should be BULK_TRANSFER_ACK
//     uint8_t transaction_id;
//     CdmpResultCode result;
//     uint8_t reserved[3];
// };

} // namespace eerie_leap::subsys::cdmp::models
