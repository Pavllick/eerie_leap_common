#pragma once

#include <cstdint>
#include <chrono>
#include <zephyr/kernel.h>

#include "../types/cdmp_types.h"

namespace eerie_leap::subsys::cdmp::models {

using namespace eerie_leap::subsys::cdmp::types;

enum class CdmpDeviceType : uint8_t {
    NONE = 0x00,
    LOGGER = 0x01,
    DISPLAY = 0x02
};

enum class CdmpDeviceStatus : uint8_t {
    OFFLINE = 0x00,
    INIT = 0x01,
    CLAIMING = 0x02,
    ONLINE = 0x03,
    VERSION_MISMATCH = 0x04,
    ERROR = 0x05
};

enum class CdmpDeviceCapabilityFlags : uint32_t {
    NONE = 0,
    LOGGER_SUPPORT = 1 << 0,
    DISPLAY_SUPPORT = 1 << 1,
    DATA_STORAGE = 1 << 2
};

class CdmpDevice {
private:
    uint8_t device_id_;
    uint32_t unique_identifier_;
    CdmpDeviceType device_type_;
    uint8_t protocol_version_;
    CdmpDeviceStatus status_;
    CdmpHealthStatus health_status_;
    uint32_t capability_flags_;
    uint64_t last_heartbeat_;
    uint8_t uptime_counter_;

public:
    static constexpr uint8_t DEVICE_ID_UNASSIGNED = 0x00;
    static constexpr uint8_t DEVICE_ID_BROADCAST = 0xFF;

    CdmpDevice();
    CdmpDevice(uint32_t unique_identifier, CdmpDeviceType device_type);

    // Getters
    uint8_t GetDeviceId() const { return device_id_; }
    uint32_t GetUniqueIdentifier() const { return unique_identifier_; }
    CdmpDeviceType GetDeviceType() const { return device_type_; }
    uint8_t GetProtocolVersion() const { return protocol_version_; }
    CdmpDeviceStatus GetStatus() const { return status_; }
    CdmpHealthStatus GetHealthStatus() const { return health_status_; }
    uint32_t GetCapabilityFlags() const { return capability_flags_; }
    uint64_t GetLastHeartbeat() const { return last_heartbeat_; }
    uint8_t GetUptimeCounter() const { return uptime_counter_; }

    // Setters
    void SetDeviceId(uint8_t device_id) { device_id_ = device_id; }
    void SetStatus(CdmpDeviceStatus status) { status_ = status; }
    void SetHealthStatus(CdmpHealthStatus health_status) { health_status_ = health_status; }
    void SetCapabilityFlags(uint32_t flags) { capability_flags_ = flags; }
    void UpdateHeartbeat();
    void IncrementUptime() { uptime_counter_++; }

    // Utility methods
    bool IsOnline() const { return status_ == CdmpDeviceStatus::ONLINE; }
    bool HasCapability(CdmpDeviceCapabilityFlags capability) const { return (capability_flags_ & static_cast<uint32_t>(capability)) != 0; }
    void SetCapability(CdmpDeviceCapabilityFlags capability, bool enabled);

    // Protocol version compatibility
    bool IsProtocolCompatible(uint8_t other_version) const;
    static uint8_t GetCurrentProtocolVersion() { return 0x10; } // Version 1.0
};

} // namespace eerie_leap::subsys::cdmp::models
