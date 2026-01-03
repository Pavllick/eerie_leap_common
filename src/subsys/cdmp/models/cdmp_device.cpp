#include <utility>

#include <zephyr/logging/log.h>

#include "cdmp_device.h"

LOG_MODULE_REGISTER(cdmp_device, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::models {

CdmpDevice::CdmpDevice(uint32_t unique_identifier, CdmpDeviceType device_type, CdmpDeviceStatus status)
    : protocol_version_(GetCurrentProtocolVersion()),
    status_machine_(std::make_shared<CdmpStatusMachine>(status)) {}


void CdmpDevice::StartDiscovery() {
    SetStatus(CdmpDeviceStatus::INIT);
    LOG_INF("Starting device discovery");
}

void CdmpDevice::ClaimId() {
    SetStatus(CdmpDeviceStatus::CLAIMING);
    LOG_INF("Claiming device ID");
}

void CdmpDevice::GoOnline() {
    SetStatus(CdmpDeviceStatus::ONLINE);
    LOG_INF("Device is now online");
}

void CdmpDevice::EnterVersionMismatch() {
    SetStatus(CdmpDeviceStatus::VERSION_MISMATCH);
    LOG_ERR("Protocol version mismatch detected");
}

void CdmpDevice::EnterError() {
    SetStatus(CdmpDeviceStatus::ERROR);
    LOG_ERR("Device entered error state");
}

void CdmpDevice::Reset() {
    SetStatus(CdmpDeviceStatus::OFFLINE);
    health_status_ = CdmpHealthStatus::OK;
    LOG_INF("Device reset to default state");
}

void CdmpDevice::SetStatus(CdmpDeviceStatus status, bool force) {
    status_machine_->SetStatus(status, force);
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
