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

// Message types for Management messages (Base + 0)
enum class CdmpManagementMessageType : uint8_t {
    ID_CLAIM = 0x01,
    ID_CLAIM_RESPONSE = 0x02,
    DISCOVERY_REQUEST = 0x03,
    DISCOVERY_RESPONSE = 0x04,
    HEARTBEAT = 0x05
};

// ID Claim Response result codes for Management messages (Base + 0)
enum class CdmpIdClaimResult : uint8_t {
    ACCEPT = 0x00,
    REJECT = 0x01,
    VERSION_INCOMPATIBLE = 0x02
};

// Command codes for Command requests (Base + 2)
enum class CdmpCommandCode : uint8_t {
    READ_PARAMETER = 0x10,
    WRITE_PARAMETER = 0x11,
    EXECUTE_ACTION = 0x12,
    RESET_DEVICE = 0x13,
    STATUS_REQUEST = 0x14,
    GET_CONFIG_CRC = 0x22,
    GET_CONFIG = 0x23,
    // Application-specific: 0x30-0xFF
};

// State types for State Change Notifications (Base + 4)
enum class CdmpStateType : uint8_t {
    OPERATING_MODE_CHANGED = 0x01,
    FAULT_CONDITION_DETECTED = 0x02,
    CALIBRATION_STATUS_CHANGED = 0x03,
    CONNECTION_STATUS_CHANGED = 0x04,
    THRESHOLD_EVENT = 0x05,
    // Application-specific: 0x10-0xFF
};

// TODO: Refactor to register config types, they might be:
// SENSORS_CONFIG, CANBUS_CONFIG, etc.
// Probably get rid of the enum and use parser in a Command Handler Class
// Config types for configuration management
enum class CdmpConfigType : uint8_t {
    COMPLETE = 0x00,
    DEVICE_SETTINGS = 0x01,
    CAN_CONFIGURATION = 0x02,
    CAPABILITY_SETTINGS = 0x03,
    // Application-specific: 0x04-0xFF
};

// // ISO-TP frame types
// enum class CdmpIsoTpFrameType : uint8_t {
//     SINGLE_FRAME = 0x00,  // 0x0N where N = length (1-7)
//     FIRST_FRAME = 0x10,    // 0x1FFF where FFF = total length (12 bits)
//     CONSECUTIVE_FRAME = 0x20, // 0x2N where N = sequence number (0-15)
//     FLOW_CONTROL = 0x30   // 0x30/0x31/0x32
// };

// // ISO-TP flow control status
// enum class CdmpIsoTpFlowStatus : uint8_t {
//     CONTINUE = 0x30,
//     WAIT = 0x31,
//     ABORT = 0x32
// };

// // ISO-TP transfer types
// enum class CdmpIsoTpTransferType : uint8_t {
//     CONFIG_WRITE = 0x01,
//     CONFIG_READ = 0x02,
//     LOG_DATA = 0x03,
//     // Application-specific: 0x04-0xFF
// };

// Base message structures (Base + 0)
struct CdmpManagementMessage {
    CdmpManagementMessageType message_type;
    uint8_t device_id;
    uint32_t unique_identifier;
    CdmpDeviceType device_type;
    uint8_t protocol_version;

    // Heartbeat specific fields
    CdmpHealthStatus health_status;
    uint8_t uptime_counter;
    uint32_t capability_flags;
};

struct CdmpHeartbeatMessage {
    const CdmpManagementMessageType message_type = CdmpManagementMessageType::HEARTBEAT;
    uint8_t device_id;
    CdmpHealthStatus health_status;
    uint8_t sequence_number;
    uint32_t capability_flags;

    static CdmpHeartbeatMessage FromCanFrame(std::span<const uint8_t> frame_data) {
        if(frame_data[0] != static_cast<uint8_t>(CdmpManagementMessageType::HEARTBEAT))
            throw std::invalid_argument("Incorrect message type");

        CdmpHeartbeatMessage heartbeat = {};
        heartbeat.device_id = frame_data[1];
        heartbeat.health_status = static_cast<CdmpHealthStatus>(frame_data[2]);
        heartbeat.sequence_number = frame_data[3];
        // Extract 32-bit capability flags (LSB first)
        heartbeat.capability_flags =
            (static_cast<uint32_t>(frame_data[7]) << 24)
            | (static_cast<uint32_t>(frame_data[6]) << 16)
            | (static_cast<uint32_t>(frame_data[5]) << 8)
            | static_cast<uint32_t>(frame_data[4]);

        return heartbeat;
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
        CdmpCommandMessage command = {};
        command.target_device_id = frame_data[0];
        command.command_code = static_cast<CdmpCommandCode>(frame_data[1]);
        command.transaction_id = frame_data[2];
        command.data = std::vector<uint8_t>(frame_data.begin() + 3, frame_data.end());

        return command;
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
        eerie_leap::subsys::cdmp::models::CdmpCommandResponse response = {};
        response.source_device_id = frame_data[0];
        response.command_code = static_cast<CdmpCommandCode>(frame_data[1]);
        response.transaction_id = frame_data[2];
        response.result_code = static_cast<CdmpResultCode>(frame_data[3]);
        response.data = std::vector<uint8_t>(frame_data.begin() + 4, frame_data.end());

        return response;
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
        CdmpStateChangeNotification command = {};
        command.source_device_id = frame_data[0];
        command.state_version_number = frame_data[1];
        command.state_type = static_cast<CdmpStateType>(frame_data[2]);
        command.data = std::vector<uint8_t>(frame_data.begin() + 3, frame_data.end());

        return command;
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
        CdmpStateChangeResponse command = {};
        command.source_device_id = frame_data[0];
        command.target_device_id = frame_data[1];
        command.state_version_number = frame_data[2];
        command.response_code = static_cast<CdmpResultCode>(frame_data[3]);
        command.data = std::vector<uint8_t>(frame_data.begin() + 4, frame_data.end());

        return command;
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

class CdmpCanIdManager {
private:
    uint32_t base_can_id_;

public:
    static constexpr uint32_t DEFAULT_BASE_CAN_ID = 0x700;
    static constexpr uint32_t MANAGEMENT_OFFSET = 0;
    static constexpr uint32_t STATUS_OFFSET = 1;
    static constexpr uint32_t COMMAND_OFFSET = 2;
    static constexpr uint32_t COMMAND_RESPONSE_OFFSET = 3;
    static constexpr uint32_t STATE_CHANGE_OFFSET = 4;
    static constexpr uint32_t STATE_CHANGE_RESPONSE_OFFSET = 5;
    static constexpr uint32_t ISOTP_REQUEST_OFFSET = 6;
    static constexpr uint32_t ISOTP_RESPONSE_OFFSET = 7;
    static constexpr uint32_t CAPABILITY_OFFSET_START = 20;
    static constexpr uint32_t CAPABILITY_OFFSET_END = 51;
    static constexpr uint32_t APPLICATION_OFFSET_START = 52;
    static constexpr uint32_t APPLICATION_OFFSET_END = 99;

    CdmpCanIdManager(uint32_t base_can_id = DEFAULT_BASE_CAN_ID)
        : base_can_id_(base_can_id) {}

    uint32_t GetBaseCanId() const { return base_can_id_; }
    uint32_t GetManagementCanId() const { return base_can_id_ + MANAGEMENT_OFFSET; }
    uint32_t GetHeartbeatCanId() const { return GetManagementCanId(); }
    uint32_t GetStatusCanId() const { return base_can_id_ + STATUS_OFFSET; }
    uint32_t GetCommandCanId() const { return base_can_id_ + COMMAND_OFFSET; }
    uint32_t GetCommandResponseCanId() const { return base_can_id_ + COMMAND_RESPONSE_OFFSET; }
    uint32_t GetStateChangeCanId() const { return base_can_id_ + STATE_CHANGE_OFFSET; }
    uint32_t GetStateChangeResponseCanId() const { return base_can_id_ + STATE_CHANGE_RESPONSE_OFFSET; }
    uint32_t GetIsoTpRequestCanId() const { return base_can_id_ + ISOTP_REQUEST_OFFSET; }
    uint32_t GetIsoTpResponseCanId() const { return base_can_id_ + ISOTP_RESPONSE_OFFSET; }

    uint32_t GetCapabilityCanId(uint8_t capability_bit) const {
        if(capability_bit > (CAPABILITY_OFFSET_END - CAPABILITY_OFFSET_START))
            throw std::invalid_argument("capability_bit out of range");

        return base_can_id_ + CAPABILITY_OFFSET_START + capability_bit;
    }
};

} // namespace eerie_leap::subsys::cdmp::models
