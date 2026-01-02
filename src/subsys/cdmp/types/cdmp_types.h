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

} // namespace eerie_leap::subsys::cdmp::types
