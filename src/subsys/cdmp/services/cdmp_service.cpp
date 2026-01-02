#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "../models/cdmp_device.h"
#include "cdmp_management_service/cdmp_management_service.h"
#include "cdmp_status_service.h"
#include "cdmp_command_service/cdmp_command_service.h"
#include "cdmp_state_service.h"

#include "cdmp_service.h"

LOG_MODULE_REGISTER(cdmp_service, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::services {

using namespace eerie_leap::subsys::cdmp::services::cdmp_management_service;
using namespace eerie_leap::subsys::cdmp::services::cdmp_command_service;

CdmpService::CdmpService(
    std::shared_ptr<Canbus> canbus,
    CdmpDeviceType device_type,
    uint32_t unique_identifier,
    uint32_t base_can_id)
    : canbus_(std::move(canbus))
    , next_transaction_id_(1)
    , base_can_id_(base_can_id)
    , device_type_(device_type)
    , unique_identifier_(unique_identifier)
    , auto_discovery_enabled_(true)
    , heartbeat_interval_(DEFAULT_HEARTBEAT_INTERVAL)
    , last_status_broadcast_(0)
    , status_broadcast_interval_(DEFAULT_STATUS_BROADCAST_INTERVAL)
    , status_sequence_number_(0) {

    // Generate unique identifier if not provided
    if(unique_identifier_ == DEFAULT_UNIQUE_IDENTIFIER)
        unique_identifier_ = sys_rand32_get();

    can_id_manager_ = std::make_shared<CdmpCanIdManager>();
    device_ = std::make_shared<CdmpDevice>(unique_identifier_, device_type_);
    status_machine_ = std::make_shared<CdmpStatusMachine>(device_);

    canbus_services_.emplace_back(std::make_unique<CdmpManagementService>(
        canbus_, can_id_manager_, device_, status_machine_));
    canbus_services_.emplace_back(std::make_unique<CdmpStatusService>(
        canbus_, can_id_manager_, device_, status_machine_));
    canbus_services_.emplace_back(std::make_unique<CdmpCommandService>(
        canbus_, can_id_manager_, device_, status_machine_));
    canbus_services_.emplace_back(std::make_unique<CdmpStateService>(
        canbus_, can_id_manager_, device_, status_machine_));
    // TODO: Add IsoTp Service
}

CdmpService::~CdmpService() {
    Stop();
}

bool CdmpService::Initialize() {
    if(!canbus_) {
        LOG_ERR("CAN bus interface is null");
        return false;
    }

    status_machine_->Initialize();

    LOG_INF("CDMP service initialized with device type %d, unique ID 0x%08X",
        std::to_underlying(device_type_), unique_identifier_);

    return true;
}

void CdmpService::Start() {
    if(!work_queue_thread_) {
        work_queue_thread_ = std::make_unique<WorkQueueThread>(
            "cdmp_service", thread_stack_size_, thread_priority_);
        work_queue_thread_->Initialize();
    }

    for(auto& service : canbus_services_)
        service->Start();

    if(auto_discovery_enabled_)
        status_machine_->StartDiscovery();

    LOG_INF("CDMP service started");
}

void CdmpService::Stop() {
    // WorkQueueThread destructor handles stopping the work queue
    work_queue_thread_.reset();

    for(auto& service : canbus_services_)
        service->Stop();

    if(device_)
        device_->SetStatus(CdmpDeviceStatus::OFFLINE);

    LOG_INF("CDMP service stopped");
}

bool CdmpService::IsRunning() const {
    return work_queue_thread_ != nullptr;
}

uint8_t CdmpService::GetNextTransactionId() {
    uint8_t transaction_id = next_transaction_id_;
    next_transaction_id_ = (next_transaction_id_ + 1) % 256;
    if(next_transaction_id_ == 0)
        next_transaction_id_ = 1; // Skip 0

    return transaction_id;
}

void CdmpService::StartTransaction(uint8_t transaction_id) {
    pending_transactions_[transaction_id] = k_uptime_get();
}

void CdmpService::CompleteTransaction(uint8_t transaction_id) {
    pending_transactions_.erase(transaction_id);
}

void CdmpService::CleanupExpiredTransactions() {
    uint64_t current_time = k_uptime_get();

    auto it = pending_transactions_.begin();
    while(it != pending_transactions_.end()) {
        if(current_time > (it->second + TRANSACTION_TIMEOUT)) {
            LOG_WRN("Transaction %d timed out", it->first);
            it = pending_transactions_.erase(it);
        } else {
            ++it;
        }
    }
}

void CdmpService::SetHeartbeatInterval(uint64_t interval) {
    heartbeat_interval_ = interval;
}

void CdmpService::SetAutoDiscovery(bool enabled) {
    auto_discovery_enabled_ = enabled;
}

std::vector<uint8_t> CdmpService::GetOnlineDeviceIds() const {
    std::vector<uint8_t> online_ids;

    for(const auto& pair : network_devices_) {
        if(pair.second && pair.second->IsOnline())
            online_ids.push_back(pair.first);
    }

    return online_ids;
}

CdmpDevice& CdmpService::GetDevice() const {
    return *device_;
}

const CdmpDevice* CdmpService::GetDevice(uint8_t device_id) const {
    if(!network_devices_.contains(device_id))
        return nullptr;

    return network_devices_.at(device_id).get();
}

void CdmpService::PrintNetworkStatus() const {
    LOG_INF("CDMP Network Status:");
    LOG_INF("  Local Device ID: %d", device_->GetDeviceId());
    LOG_INF("  Local Device Status: %d", static_cast<int>(device_->GetStatus()));
    LOG_INF("  Online Devices: %zu", network_devices_.size());

    for(const auto& pair : network_devices_) {
        const auto& device = pair.second;
        if(device) {
            LOG_INF("    Device %d: Status=%d, Type=%d, UID=0x%08X",
                    pair.first, static_cast<int>(device->GetStatus()),
                    std::to_underlying(device->GetDeviceType()), device->GetUniqueIdentifier());
        }
    }
}

void CdmpService::PrintDeviceStatus() const {
    if(!device_) {
        LOG_INF("Device not initialized");
        return;
    }

    LOG_INF("CDMP Device Status:");
    LOG_INF("  Device ID: %d", device_->GetDeviceId());
    LOG_INF("  Device Type: %d", std::to_underlying(device_->GetDeviceType()));
    LOG_INF("  Unique ID: 0x%08X", device_->GetUniqueIdentifier());
    LOG_INF("  Protocol Version: 0x%02X", device_->GetProtocolVersion());
    LOG_INF("  Status: %d", static_cast<int>(device_->GetStatus()));
    LOG_INF("  Health: %d", static_cast<int>(device_->GetHealthStatus()));
    LOG_INF("  Capability Flags: 0x%08X", device_->GetCapabilityFlags());
    LOG_INF("  Uptime: %d", device_->GetUptimeCounter());
}

WorkQueueTaskResult CdmpService::ProcessCdmpTask(void* task_data) {
    // This would be implemented to handle periodic tasks
    return WorkQueueTaskResult{};
}

} // namespace eerie_leap::subsys::cdmp::services
