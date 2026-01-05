#include "cdmp_management_service.h"

namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

CdmpManagementService::CdmpManagementService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device,
    std::shared_ptr<ITimeService> time_service,
    std::shared_ptr<WorkQueueThread> work_queue_thread)
        : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device)),
        time_service_(std::move(time_service)),
        work_queue_thread_(std::move(work_queue_thread)) {

    auto network_service = std::make_shared<CdmpNetworkService>(
        canbus_, can_id_manager_, device_, time_service_, work_queue_thread_);

    canbus_services_.push_back(network_service);
    canbus_services_.push_back(std::make_shared<CdmpHeartbeatService>(
        canbus_, can_id_manager_, device_, work_queue_thread_, network_service));
}

CdmpManagementService::~CdmpManagementService() {
    Stop();
}

void CdmpManagementService::Start() {
    for(auto& service : canbus_services_)
        service->Start();

    RegisterCanHandlers();
}

void CdmpManagementService::Stop() {
    for(auto& service : canbus_services_)
        service->Stop();

    UnregisterCanHandlers();
}

void CdmpManagementService::RegisterCanHandlers() {
    canbus_handler_id_ = canbus_->RegisterFrameReceivedHandler(
        can_id_manager_->GetManagementCanId(),
        [this](const CanFrame& frame) { ProcessFrame(frame.id, frame.data); });

    if(canbus_handler_id_ < 0) {
        throw std::runtime_error("Failed to register CAN frame handler for frame ID: "
            + std::to_string(can_id_manager_->GetManagementCanId()));
    }
}

void CdmpManagementService::UnregisterCanHandlers() {
    if(!canbus_)
        return;

    if(canbus_handler_id_ >= 0)
        canbus_->RemoveFrameReceivedHandler(can_id_manager_->GetManagementCanId(), canbus_handler_id_);
    canbus_handler_id_ = -1;
}

void CdmpManagementService::ProcessFrame(uint32_t frame_id, std::span<const uint8_t> frame_data) {
    if(frame_data.size() < 1)
        return;

    for(auto& service : canbus_services_)
        service->ProcessFrame(frame_id, frame_data);
}

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
