#include <utility>

#include <zephyr/logging/log.h>

#include "cdmp_device.h"

LOG_MODULE_REGISTER(cdmp_device, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::models {

CdmpDevice::CdmpDevice()
    : device_id_(DEVICE_ID_UNASSIGNED)
    , unique_identifier_(0)
    , device_type_(CdmpDeviceType::NONE)
    , protocol_version_(GetCurrentProtocolVersion())
    , status_(CdmpDeviceStatus::OFFLINE)
    , health_status_(CdmpHealthStatus::OK)
    , capability_flags_(0)
    , last_heartbeat_(0)
    , uptime_counter_(0) {
}

CdmpDevice::CdmpDevice(uint32_t unique_identifier, CdmpDeviceType device_type)
    : device_id_(DEVICE_ID_UNASSIGNED)
    , unique_identifier_(unique_identifier)
    , device_type_(device_type)
    , protocol_version_(GetCurrentProtocolVersion())
    , status_(CdmpDeviceStatus::OFFLINE)
    , health_status_(CdmpHealthStatus::OK)
    , capability_flags_(0)
    , last_heartbeat_(0)
    , uptime_counter_(0) {
}

void CdmpDevice::UpdateHeartbeat() {
    last_heartbeat_ = k_uptime_get();
}

void CdmpDevice::SetCapability(CdmpDeviceCapabilityFlags capability, bool enabled) {
    if (enabled) {
        capability_flags_ |= std::to_underlying(capability);
    } else {
        capability_flags_ &= ~std::to_underlying(capability);
    }
}

bool CdmpDevice::IsProtocolCompatible(uint8_t other_version) const {
    uint8_t my_major = (protocol_version_ >> 4) & 0x0F;
    uint8_t my_minor = protocol_version_ & 0x0F;
    uint8_t other_major = (other_version >> 4) & 0x0F;
    uint8_t other_minor = other_version & 0x0F;

    // Major version must match for compatibility
    return my_major == other_major;
}

} // namespace eerie_leap::subsys::cdmp::models
