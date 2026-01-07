#pragma once

#include <cstdint>
#include <stdexcept>

namespace eerie_leap::subsys::cdmp::utilities {

class CdmpCanIdManager {
private:
    uint32_t base_can_id_;

public:
    static constexpr uint32_t DEFAULT_BASE_CAN_ID = 0x700;
    static constexpr uint32_t MANAGEMENT_OFFSET = 0;
    static constexpr uint32_t HEARTBEAT_OFFSET = 1;
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
    uint32_t GetDiscoveryRequestCanId() const { return GetManagementCanId(); }
    uint32_t GetDiscoveryResponseCanId() const { return GetManagementCanId(); }

    uint32_t GetHeartbeatCanId() const { return base_can_id_ + HEARTBEAT_OFFSET; }

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

} // namespace eerie_leap::subsys::cdmp::utilities
