#include "cdmp_canbus_service_base.h"

namespace eerie_leap::subsys::cdmp::services {

CdmpCanbusServiceBase::CdmpCanbusServiceBase(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device)
        : canbus_(std::move(canbus)),
        can_id_manager_(std::move(can_id_manager)),
        device_(std::move(device)) {

    status_handler_id_ = device_->GetStatusMachine().RegisterStatusChangeHandler(
        [this](CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) {
            OnDeviceStatusChanged(old_status, new_status);
        });

    if(status_handler_id_ < 0)
        throw std::runtime_error("Failed to register status change handler");
}

CdmpCanbusServiceBase::~CdmpCanbusServiceBase() {
    if(status_handler_id_ >= 0)
        device_->GetStatusMachine().UnregisterStatusChangeHandler(status_handler_id_);
}

void CdmpCanbusServiceBase::OnDeviceStatusChanged(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) {}

void CdmpCanbusServiceBase::ProcessFrame(uint32_t frame_id, std::span<const uint8_t> frame_data) {}

} // namespace eerie_leap::subsys::cdmp::services
