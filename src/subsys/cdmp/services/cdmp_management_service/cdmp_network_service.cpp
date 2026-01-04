#include "cdmp_network_service.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cdmp_network_service, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::services {

CdmpNetworkService::CdmpNetworkService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device)
    : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device)) {}

CdmpNetworkService::~CdmpNetworkService() {
    Stop();
}

void CdmpNetworkService::Start() {
    LOG_INF("CDMP Network Service started");
}

void CdmpNetworkService::Stop() {
    ClearAllDevices();
    LOG_INF("CDMP Network Service stopped");
}

void CdmpNetworkService::OnDeviceStatusChanged(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) {
    switch(new_status) {
        case CdmpDeviceStatus::INIT:
            SendDiscoveryRequest();
            break;

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

void CdmpNetworkService::ProcessFrame(std::span<const uint8_t> frame_data) {
    CdmpManagementMessageType message_type = static_cast<CdmpManagementMessageType>(frame_data[0]);

    switch(message_type) {
        case CdmpManagementMessageType::DISCOVERY_REQUEST:
            LOG_INF("Received discovery request");
            SendDiscoveryResponse();
            break;

        case CdmpManagementMessageType::DISCOVERY_RESPONSE:
            LOG_INF("Received discovery response");
            ProcessDiscoveryFrame(frame_data);
            break;

        case CdmpManagementMessageType::ID_CLAIM:
            // ProcessIdClaimFrame(frame_data);
            break;

        case CdmpManagementMessageType::ID_CLAIM_RESPONSE:
            // ProcessIdClaimResponseFrame(frame_data);
            break;
    }
}

void CdmpNetworkService::ProcessDiscoveryFrame(std::span<const uint8_t> frame_data) {
    try {
        CdmpDiscoveryResponseMessage discovery = CdmpDiscoveryResponseMessage::FromCanFrame(frame_data);
        UpdateDeviceFromDiscovery(discovery);
        LOG_DBG("Processed discovery frame");
    } catch (const std::exception& e) {
        LOG_ERR("Error processing discovery frame: %s", e.what());
    }
}

void CdmpNetworkService::SendDiscoveryRequest() {
    CdmpDiscoveryRequestMessage message{};
    auto frame_data = message.ToCanFrame();
    uint32_t frame_id = can_id_manager_->GetDiscoveryRequestCanId();
    canbus_->SendFrame(frame_id, frame_data);
}

void CdmpNetworkService::SendDiscoveryResponse() {
    if(!device_->IsOnline())
        return;

    CdmpDiscoveryResponseMessage message = {};
    message.device_id = device_->GetDeviceId();
    message.unique_identifier = device_->GetUniqueIdentifier();
    message.device_type = device_->GetDeviceType();

    auto frame_data = message.ToCanFrame();
    uint32_t frame_id = can_id_manager_->GetDiscoveryResponseCanId();
    canbus_->SendFrame(frame_id, frame_data);

    LOG_INF("Sent discovery response for device %d", message.device_id);
}

void CdmpNetworkService::SendIdClaim() {
    CdmpIdClaimMessage message = {};
    message.claimed_device_id = device_->GetDeviceId();
    message.unique_identifier = device_->GetUniqueIdentifier();
    message.device_type = device_->GetDeviceType();
    message.protocol_version = device_->GetProtocolVersion();

    auto frame_data = message.ToCanFrame();
    uint32_t frame_id = can_id_manager_->GetManagementCanId();
    canbus_->SendFrame(frame_id, frame_data);

    LOG_INF("Sent ID claim for device %d", message.claimed_device_id);
}

void CdmpNetworkService::UpdateDeviceFromDiscovery(const CdmpDiscoveryResponseMessage& discovery) {
    AddOrUpdateDevice(discovery.device_id, discovery.device_type, discovery.unique_identifier);
}

void CdmpNetworkService::UpdateDeviceFromHeartbeat(const CdmpHeartbeatMessage& heartbeat) {
    uint8_t device_id = heartbeat.device_id;

    if(!network_devices_.contains(device_id)) {
        LOG_DBG("Heartbeat from unknown device %d", device_id);
        return;
    }

    AddOrUpdateDevice(
        heartbeat.device_id,
        network_devices_.at(device_id)->GetDeviceType(),
        network_devices_.at(device_id)->GetUniqueIdentifier());
}

void CdmpNetworkService::AddOrUpdateDevice(uint8_t device_id, CdmpDeviceType device_type, uint32_t unique_identifier) {
    if(network_devices_.contains(device_id)) {
        network_devices_.at(device_id)->SetStatus(CdmpDeviceStatus::ONLINE, true);
        network_devices_.at(device_id)->UpdateHeartbeat();

        LOG_DBG("Updated device %d.", device_id);
    } else {
        auto device = std::make_unique<CdmpDevice>(unique_identifier, device_type, CdmpDeviceStatus::ONLINE);
        device->SetDeviceId(device_id);
        network_devices_.emplace(device_id, std::move(device));

        LOG_INF("Added new device: ID=%d, Type=%d, UID=0x%08X", device_id, std::to_underlying(device_type), unique_identifier);
    }
}

void CdmpNetworkService::RemoveDevice(uint8_t device_id) {
    if(network_devices_.contains(device_id)) {
        network_devices_.erase(device_id);
        LOG_INF("Removed device %d", device_id);
    }
}

void CdmpNetworkService::ClearAllDevices() {
    network_devices_.clear();
    LOG_INF("Cleared all network devices");
}

std::vector<uint8_t> CdmpNetworkService::GetOnlineDeviceIds() const {
    std::vector<uint8_t> online_devices;
    for(const auto& [device_id, device] : network_devices_) {
        if(device->GetStatus() == CdmpDeviceStatus::ONLINE)
            online_devices.push_back(device_id);
    }
    return online_devices;
}

std::vector<uint8_t> CdmpNetworkService::GetAllDeviceIds() const {
    std::vector<uint8_t> all_devices;
    for(const auto& [device_id, device] : network_devices_)
        all_devices.push_back(device_id);

    return all_devices;
}

const CdmpDevice* CdmpNetworkService::GetDevice(uint8_t device_id) const {
    if(!network_devices_.contains(device_id))
        return nullptr;

    return network_devices_.at(device_id).get();
}

CdmpDevice* CdmpNetworkService::GetDevice(uint8_t device_id) {
    if(!network_devices_.contains(device_id))
        return nullptr;

    return network_devices_.at(device_id).get();
}

void CdmpNetworkService::SetAutoDiscovery(bool enabled) {
    auto_discovery_enabled_ = enabled;
    LOG_INF("Auto discovery %s", enabled ? "enabled" : "disabled");
}

void CdmpNetworkService::ProcessPeriodicTasks() {
    // uint64_t current_time = k_uptime_get_64();

    // // Periodic discovery scan
    // if(auto_discovery_enabled_ &&
    //     (current_time - last_discovery_scan_ >= DISCOVERY_SCAN_INTERVAL)) {
    //     ScanForDevices();
    // }

    // // Remove offline devices
    // RemoveOfflineDevices();
}

bool CdmpNetworkService::IsDeviceOnline(uint8_t device_id) const {
    const auto* device = GetDevice(device_id);
    return device && device->GetStatus() == CdmpDeviceStatus::ONLINE;
}

std::vector<uint8_t> CdmpNetworkService::GetOfflineDeviceIds() const {
    std::vector<uint8_t> offline_devices;
    for(const auto& [device_id, device] : network_devices_) {
        if(device->GetStatus() != CdmpDeviceStatus::ONLINE)
            offline_devices.push_back(device_id);
    }

    return offline_devices;
}

void CdmpNetworkService::RemoveOfflineDevices() {
    auto offline_device_ids = GetOfflineDeviceIds();

    for(auto device_id : offline_device_ids)
        RemoveDevice(device_id);
}

void CdmpNetworkService::PrintNetworkStatus() const {
    LOG_INF("Network Status: %zu devices total", network_devices_.size());

    for(const auto& [device_id, device] : network_devices_) {
        LOG_INF("  Device %d: Type=%d, Status=%d, UID=0x%08X",
            device_id, std::to_underlying(device->GetDeviceType()),
            std::to_underlying(device->GetStatus()), device->GetUniqueIdentifier());
    }
}

} // namespace eerie_leap::subsys::cdmp::services
