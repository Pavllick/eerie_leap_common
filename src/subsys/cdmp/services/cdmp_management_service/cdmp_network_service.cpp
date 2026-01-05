#include <zephyr/logging/log.h>

#include "../../types/cdmp_types.h"
#include "subsys/cdmp/utilities/cdmp_status_machine.h"

#include "cdmp_network_service.h"

LOG_MODULE_REGISTER(cdmp_network_service, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::services {

using namespace eerie_leap::subsys::cdmp::types;

CdmpNetworkService::CdmpNetworkService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device,
    std::shared_ptr<CdmpWorkQueue> work_queue)
        : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device)),
        work_queue_(work_queue) {}

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
            work_queue_->Run([this]() { StartInitialization(); });
            break;

        case CdmpDeviceStatus::CLAIMING:
            work_queue_->Run([this]() { SendIdClaim(); });
            break;

        case CdmpDeviceStatus::ONLINE:
            LOG_INF("Device is now online");
            break;

        case CdmpDeviceStatus::VERSION_MISMATCH:
            LOG_ERR("Protocol version mismatch detected");
            break;

        case CdmpDeviceStatus::ERROR:
            LOG_ERR("Device entered error state");
            break;

        default:
            break;
    }
}

void CdmpNetworkService::ProcessFrame(uint32_t frame_id, std::span<const uint8_t> frame_data) {
    CdmpManagementMessageType message_type = static_cast<CdmpManagementMessageType>(frame_data[0]);

    switch(message_type) {
        case CdmpManagementMessageType::DISCOVERY_REQUEST:
            LOG_INF("Received discovery request");
            SendDiscoveryResponse();
            break;

        case CdmpManagementMessageType::DISCOVERY_RESPONSE:
            LOG_INF("Received discovery response");
            ProcessDiscoveryResponseFrame(frame_data);
            break;

        case CdmpManagementMessageType::ID_CLAIM:
            ProcessIdClaimRequestFrame(frame_data);
            break;

        case CdmpManagementMessageType::ID_CLAIM_RESPONSE:
            ProcessIdClaimResponseFrame(frame_data);
            break;
    }
}

void CdmpNetworkService::StartInitialization() {
    discovery_response_received_ = false;

    for(const auto& backoff : CdmpTimeouts::RETRY_BACKOFFS_MS) {
        SendDiscoveryRequest();
        k_msleep(200);

        if(discovery_response_received_)
            break;

        LOG_DBG("Retry discovery attempt with backoff: %d ms", backoff);
    }

    device_->SetStatus(CdmpDeviceStatus::CLAIMING);
}

void CdmpNetworkService::ProcessDiscoveryResponseFrame(std::span<const uint8_t> frame_data) {
    try {
        CdmpDiscoveryResponseMessage discovery = CdmpDiscoveryResponseMessage::FromCanFrame(frame_data);
        UpdateDeviceFromDiscovery(discovery);
        LOG_DBG("Processed discovery frame");

        discovery_response_received_ = true;
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
    try {
        claiming_device_id_ = GetLowestAvailableId(claiming_device_id_);
    } catch (const std::exception& e) {
        LOG_ERR("Error getting lowest available ID: %s", e.what());
        device_->SetStatus(CdmpDeviceStatus::ERROR);
        claiming_device_id_ = 0;

        return;
    }

    for(int i = 0; i < ID_CLAIM_RETRY_COUNT; ++i) {
        CdmpIdClaimMessage message = {};
        message.claiming_device_id = claiming_device_id_;
        message.unique_identifier = device_->GetUniqueIdentifier();
        message.device_type = device_->GetDeviceType();
        message.protocol_version = device_->GetProtocolVersion();

        auto frame_data = message.ToCanFrame();
        uint32_t frame_id = can_id_manager_->GetManagementCanId();
        canbus_->SendFrame(frame_id, frame_data);
        LOG_INF("Sent ID claim for device %d", message.claiming_device_id);

        k_msleep(ID_CLAIM_RESPONSE_TIMEOUT_MS);

        if(id_claim_result_.has_value() && id_claim_result_.value() == CdmpIdClaimResult::VERSION_INCOMPATIBLE) {
            device_->SetStatus(CdmpDeviceStatus::VERSION_MISMATCH);
            claiming_device_id_ = 0;
            return;
        }

        if(id_claim_result_.has_value() && id_claim_result_.value() == CdmpIdClaimResult::ACCEPT) {
            device_->SetDeviceId(claiming_device_id_);
            device_->SetStatus(CdmpDeviceStatus::ONLINE);
            break;
        }
    }

    // First or only device on the network, assign the ID
    if(!id_claim_result_.has_value()) {
        device_->SetDeviceId(claiming_device_id_);
        device_->SetStatus(CdmpDeviceStatus::ONLINE);
    } else if(id_claim_result_.has_value() && id_claim_result_.value() == CdmpIdClaimResult::REJECT) {
        LOG_ERR("All ID claims were rejected.");
        device_->SetStatus(CdmpDeviceStatus::ERROR);
    }

    claiming_device_id_ = 0;
}

void CdmpNetworkService::ProcessIdClaimRequestFrame(std::span<const uint8_t> frame_data) {
    try {
        if(!device_->IsOnline())
            return;

        CdmpIdClaimMessage id_claim = CdmpIdClaimMessage::FromCanFrame(frame_data);
        LOG_INF("Received ID claim from device %d", id_claim.claiming_device_id);

        if(device_->GetDeviceId() == lowest_id_on_network_
           && id_claim.protocol_version != device_->GetProtocolVersion()) {

            CdmpIdClaimResponseMessage message = {};
            message.responding_device_id = device_->GetDeviceId();
            message.claiming_device_id = id_claim.claiming_device_id;
            message.result = CdmpIdClaimResult::VERSION_INCOMPATIBLE;

            auto frame_data = message.ToCanFrame();
            uint32_t response_id = can_id_manager_->GetManagementCanId();
            canbus_->SendFrame(response_id, frame_data);

            return;
        }

        if(device_->GetDeviceId() == id_claim.claiming_device_id) {
            CdmpIdClaimResponseMessage message = {};
            message.responding_device_id = device_->GetDeviceId();
            message.claiming_device_id = id_claim.claiming_device_id;
            message.result = CdmpIdClaimResult::REJECT;

            auto frame_data = message.ToCanFrame();
            uint32_t response_id = can_id_manager_->GetManagementCanId();
            canbus_->SendFrame(response_id, frame_data);

            return;
        }
    } catch (const std::exception& e) {
        LOG_ERR("Error processing ID claim frame: %s", e.what());
    }
}

void CdmpNetworkService::ProcessIdClaimResponseFrame(std::span<const uint8_t> frame_data) {
    try {
        CdmpIdClaimResponseMessage response = CdmpIdClaimResponseMessage::FromCanFrame(frame_data);

        if(device_->IsOnline())
            return;

        if(response.claiming_device_id != claiming_device_id_)
            return;

        LOG_INF("Received ID claim response from device %d for device %d with result %d",
            response.responding_device_id, response.claiming_device_id, static_cast<int>(response.result));

        id_claim_result_ = response.result;
    } catch (const std::exception& e) {
        LOG_ERR("Error processing ID claim response frame: %s", e.what());
    }
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

        if(device_id < device_->GetDeviceId())
            lowest_id_on_network_ = device_id;

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

    // Update lowest_id_on_network_
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

uint8_t CdmpNetworkService::GetLowestAvailableId(uint8_t after) const {
    for(uint8_t i = after + 1; i <= 255; ++i) {
        if(!network_devices_.contains(i))
            return i;
    }

    throw std::runtime_error("No available device ID found");
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
