#include "cdmp_status_service.h"

namespace eerie_leap::subsys::cdmp::services {

CdmpStatusService::CdmpStatusService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device)
        : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device)),
        canbus_handler_id_(-1) {}

CdmpStatusService::~CdmpStatusService() {
    Stop();
}

void CdmpStatusService::Start() {
    RegisterCanHandlers();
}

void CdmpStatusService::Stop() {
    UnregisterCanHandlers();
}

void CdmpStatusService::RegisterCanHandlers() {
    canbus_handler_id_ = canbus_->RegisterFrameReceivedHandler(
        can_id_manager_->GetStatusCanId(),
        [this](const CanFrame& frame) { ProcessFrame(frame.data); });
}

void CdmpStatusService::UnregisterCanHandlers() {
    if(!canbus_)
        return;

    if(canbus_handler_id_ >= 0)
        canbus_->RemoveFrameReceivedHandler(can_id_manager_->GetStatusCanId(), canbus_handler_id_);
    canbus_handler_id_ = -1;
}

void CdmpStatusService::ProcessFrame(std::span<const uint8_t> frame_data) {
    // Process status broadcasts from other devices
    if (frame_data.size() >= 2) {
        uint8_t device_id = frame_data[0];
        uint8_t sequence_number = frame_data[1];

        // Update device status in network registry
        // LOG_DBG("Status broadcast from device %d, sequence %d", device_id, sequence_number);
    }
}

} // namespace eerie_leap::subsys::cdmp::services
