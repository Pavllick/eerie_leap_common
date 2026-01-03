#pragma once

#include <cstdint>

namespace eerie_leap::subsys::cdmp::types {

enum class CdmpHealthStatus : uint8_t {
    OK = 0x00,
    WARNING = 0x01,
    ERROR = 0x02
};

// Result codes for Command Response (Base + 3)
enum class CdmpResultCode : uint8_t {
    SUCCESS = 0x00,
    GENERIC_ERROR = 0x01,
    INVALID_PARAMETER = 0x02,
    UNSUPPORTED_COMMAND = 0x03,
    TIMEOUT = 0x04,
    CRC_ERROR = 0x05,
    BUFFER_OVERFLOW = 0x06,
    DEVICE_BUSY = 0x07,
    ACCESS_DENIED = 0x08,
    NOT_READY = 0x09,
    INVALID_STATE = 0x0A,

    BULK_TRANSFER_ACK = 0xF0
};

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

} // namespace eerie_leap::subsys::cdmp::types
