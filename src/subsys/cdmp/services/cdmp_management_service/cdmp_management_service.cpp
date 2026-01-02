#include "cdmp_management_service.h"

namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

CdmpManagementService::CdmpManagementService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device,
    std::shared_ptr<CdmpStatusMachine> status_machine)
        : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device), std::move(status_machine)),
        canbus_handler_id_(-1) {

    heartbeat_service_ = std::make_unique<CdmpHeartbeatService>(
        canbus_, can_id_manager_, device_, status_machine_);
}

CdmpManagementService::~CdmpManagementService() {
    Stop();
}

void CdmpManagementService::Start() {
    heartbeat_service_->Start();
    RegisterCanHandlers();
}

void CdmpManagementService::Stop() {
    heartbeat_service_->Stop();
    UnregisterCanHandlers();
}

void CdmpManagementService::RegisterCanHandlers() {
    canbus_handler_id_ = canbus_->RegisterFrameReceivedHandler(
        can_id_manager_->GetManagementCanId(),
        [this](const CanFrame& frame) { ProcessFrame(frame); });
}

void CdmpManagementService::UnregisterCanHandlers() {
    if(!canbus_)
        return;

    if(canbus_handler_id_ >= 0)
        canbus_->RemoveFrameReceivedHandler(can_id_manager_->GetManagementCanId(), canbus_handler_id_);
    canbus_handler_id_ = -1;
}

void CdmpManagementService::ProcessFrame(const CanFrame& frame) {
    if(frame.data.size() < 1)
        return;

    CdmpManagementMessageType message_type = static_cast<CdmpManagementMessageType>(frame.data[0]);

    switch(message_type) {
        case CdmpManagementMessageType::DISCOVERY_REQUEST:
            if(device_ && device_->IsOnline())
                SendDiscoveryResponse();
            break;

        case CdmpManagementMessageType::DISCOVERY_RESPONSE:
            status_machine_->OnDiscoveryResponseReceived();
            // Process discovery response to build network device list
            break;

        case CdmpManagementMessageType::ID_CLAIM:
            // Process ID claim from other devices
            break;

        case CdmpManagementMessageType::ID_CLAIM_RESPONSE:
            if(frame.data.size() >= 4) {
                uint8_t responding_id = frame.data[1];
                uint8_t claimed_id = frame.data[2];
                CdmpIdClaimResult result = static_cast<CdmpIdClaimResult>(frame.data[3]);

                if(claimed_id == device_->GetDeviceId())
                    status_machine_->OnIdClaimResponseReceived(result);
            }
            break;

        case CdmpManagementMessageType::HEARTBEAT:
            // Process heartbeat from other devices
            if (frame.data.size() >= 8) {
                CdmpManagementMessage heartbeat = {};
                heartbeat.message_type = message_type;
                heartbeat.device_id = frame.data[1];
                heartbeat.health_status = static_cast<CdmpHealthStatus>(frame.data[2]);
                heartbeat.uptime_counter = frame.data[3];
                heartbeat.capability_flags = (static_cast<uint32_t>(frame.data[4]) << 24) |
                                         (static_cast<uint32_t>(frame.data[5]) << 16) |
                                         (static_cast<uint32_t>(frame.data[6]) << 8) |
                                         static_cast<uint32_t>(frame.data[7]);

                // Update network device registry
                if (heartbeat.device_id != device_->GetDeviceId()) {
                    // Add or update device in network_devices_
                }

                status_machine_->OnHeartbeatReceived();
            }
            break;

        default:
            // LOG_WRN("Unknown management message type: 0x%02X", static_cast<uint8_t>(message_type));
            break;
    }
}

void CdmpManagementService::SendDiscoveryResponse() {
    if(!device_)
        return;

    CdmpManagementMessage msg = {};
    msg.message_type = CdmpManagementMessageType::DISCOVERY_RESPONSE;
    msg.device_id = device_->GetDeviceId();
    msg.unique_identifier = device_->GetUniqueIdentifier();
    msg.device_type = device_->GetDeviceType();
    msg.protocol_version = device_->GetProtocolVersion();

    SendManagementMessage(msg);
    // LOG_INF("Sent discovery response for device %d", msg.device_id);
}

void CdmpManagementService::SendDiscoveryRequest() {
    CdmpManagementMessage msg = {};
    msg.message_type = CdmpManagementMessageType::DISCOVERY_REQUEST;
    msg.device_id = 0; // Not used for discovery request

    SendManagementMessage(msg);
    // LOG_INF("Sent discovery request");
}

void CdmpManagementService::SendIdClaim() {
    if(!device_)
        return;

    CdmpManagementMessage msg = {};
    msg.message_type = CdmpManagementMessageType::ID_CLAIM;
    msg.device_id = device_->GetDeviceId();
    msg.unique_identifier = device_->GetUniqueIdentifier();
    msg.device_type = device_->GetDeviceType();
    msg.protocol_version = device_->GetProtocolVersion();

    SendManagementMessage(msg);
    // LOG_INF("Sent ID claim for device %d", msg.device_id);
}

void CdmpManagementService::SendManagementMessage(const CdmpManagementMessage& msg) {
    if(!canbus_)
        return;

    uint32_t frame_id = can_id_manager_->GetManagementCanId();

    std::vector<uint8_t> frame_data;
    // Build message data based on message type
    frame_data.push_back(static_cast<uint8_t>(msg.message_type));

    switch (msg.message_type) {
        case CdmpManagementMessageType::DISCOVERY_REQUEST:
            frame_data.resize(8, 0); // Pad to 8 bytes
            break;

        case CdmpManagementMessageType::DISCOVERY_RESPONSE:
        case CdmpManagementMessageType::ID_CLAIM:
            frame_data.push_back(msg.device_id);
            frame_data.push_back((msg.unique_identifier >> 24) & 0xFF);
            frame_data.push_back((msg.unique_identifier >> 16) & 0xFF);
            frame_data.push_back((msg.unique_identifier >> 8) & 0xFF);
            frame_data.push_back(msg.unique_identifier & 0xFF);
            frame_data.push_back(std::to_underlying(msg.device_type));
            frame_data.push_back(msg.protocol_version);
            break;

        case CdmpManagementMessageType::HEARTBEAT:
            frame_data.push_back(msg.device_id);
            frame_data.push_back(static_cast<uint8_t>(msg.health_status));
            frame_data.push_back(msg.uptime_counter);
            frame_data.push_back((msg.capability_flags >> 24) & 0xFF);
            frame_data.push_back((msg.capability_flags >> 16) & 0xFF);
            frame_data.push_back((msg.capability_flags >> 8) & 0xFF);
            frame_data.push_back(msg.capability_flags & 0xFF);
            break;

        default:
            // LOG_WRN("Unsupported management message type: 0x%02X",
            //     static_cast<uint8_t>(msg.message_type));
            return;
    }

    canbus_->SendFrame(frame_id, frame_data);
}

void CdmpManagementService::OnDeviceStatusChanged(CdmpDeviceStatus old_state, CdmpDeviceStatus new_state) {
    // LOG_INF("Device status changed: %d -> %d", static_cast<int>(old_state), static_cast<int>(new_state));

    switch(new_state) {
        case CdmpDeviceStatus::CLAIMING:
            SendIdClaim();
            break;

        case CdmpDeviceStatus::ONLINE:
            // LOG_INF("Device %d is now online", device_->GetDeviceId());
            break;

        case CdmpDeviceStatus::VERSION_MISMATCH:
            // LOG_ERR("Protocol version mismatch detected");
            break;

        case CdmpDeviceStatus::ERROR:
            // LOG_ERR("Device entered error state");
            break;

        default:
            break;
    }
}

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
