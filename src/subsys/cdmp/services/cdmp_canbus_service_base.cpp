#include "cdmp_canbus_service_base.h"

namespace eerie_leap::subsys::cdmp::services {

CdmpCanbusServiceBase::CdmpCanbusServiceBase(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device,
    std::shared_ptr<CdmpStatusMachine> status_machine)
        : canbus_(std::move(canbus)),
        can_id_manager_(std::move(can_id_manager)),
        device_(std::move(device)),
        status_machine_(std::move(status_machine)) {

    status_handler_id_ = status_machine_->RegisterStatusChangeHandler(
        [this](CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) {
            OnDeviceStatusChanged(old_status, new_status);
        });

    if(status_handler_id_ < 0)
        throw std::runtime_error("Failed to register status change handler");
}

CdmpCanbusServiceBase::~CdmpCanbusServiceBase() {
    if(status_handler_id_ >= 0)
        status_machine_->UnregisterStatusChangeHandler(status_handler_id_);
}

void CdmpCanbusServiceBase::OnDeviceStatusChanged(CdmpDeviceStatus old_state, CdmpDeviceStatus new_state) {}

} // namespace eerie_leap::subsys::cdmp::services
