#include "cdmp_management_service.h"

namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

CdmpManagementService::CdmpManagementService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device)
        : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device)) {

    network_service_ = std::make_shared<CdmpNetworkService>(
        canbus_, can_id_manager_, device_);
    heartbeat_service_ = std::make_unique<CdmpHeartbeatService>(
        canbus_, can_id_manager_, device_, network_service_);
}

CdmpManagementService::~CdmpManagementService() {
    Stop();
}

void CdmpManagementService::Start() {
    network_service_->Start();
    heartbeat_service_->Start();
    RegisterCanHandlers();
}

void CdmpManagementService::Stop() {
    network_service_->Stop();
    heartbeat_service_->Stop();
    UnregisterCanHandlers();
}

void CdmpManagementService::RegisterCanHandlers() {
    canbus_handler_id_ = canbus_->RegisterFrameReceivedHandler(
        can_id_manager_->GetManagementCanId(),
        [this](const CanFrame& frame) { ProcessFrame(frame.data); });

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

void CdmpManagementService::ProcessFrame(std::span<const uint8_t> frame_data) {
    if(frame_data.size() < 1)
        return;

    // CdmpManagementMessageType message_type = static_cast<CdmpManagementMessageType>(frame_data[0]);

    network_service_->ProcessFrame(frame_data);
    heartbeat_service_->ProcessFrame(frame_data);

    // switch(message_type) {
    //     case CdmpManagementMessageType::DISCOVERY_REQUEST:
    //         // SendDiscoveryResponse();
    //         break;

    //     case CdmpManagementMessageType::DISCOVERY_RESPONSE:
    //         // Process discovery response to build network device list
    //         break;

    //     case CdmpManagementMessageType::ID_CLAIM:
    //         // Process ID claim from other devices
    //         break;

    //     case CdmpManagementMessageType::ID_CLAIM_RESPONSE:
    //         break;

    //     case CdmpManagementMessageType::HEARTBEAT:
    //         // Process heartbeat from other devices
    //         break;

    //     default:
    //         // LOG_WRN("Unknown management message type: 0x%02X", static_cast<uint8_t>(message_type));
    //         break;
    // }
}

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
