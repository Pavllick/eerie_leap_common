#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>

#include "cdmp_heartbeat_service.h"

LOG_MODULE_REGISTER(cdmp_heartbeat_service, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

CdmpHeartbeatService::CdmpHeartbeatService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device,
    std::shared_ptr<CdmpNetworkService> network_service)
    : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device)),
    network_service_(std::move(network_service)) {}

CdmpHeartbeatService::~CdmpHeartbeatService() {
    Stop();
}

void CdmpHeartbeatService::Start() {
    last_heartbeat_sent_ = k_uptime_get();
    LOG_INF("CDMP Heartbeat Service started, interval: %lld ms", heartbeat_interval_);
}

void CdmpHeartbeatService::Stop() {
    heartbeat_enabled_ = false;
    LOG_INF("CDMP Heartbeat Service stopped");
}

void CdmpHeartbeatService::ProcessFrame(std::span<const uint8_t> frame_data) {
    CdmpManagementMessageType message_type = static_cast<CdmpManagementMessageType>(frame_data[0]);

    switch(message_type) {
        case CdmpManagementMessageType::HEARTBEAT:
            ProcessHeartbeatFrame(frame_data);
            break;
    }
}

void CdmpHeartbeatService::ProcessHeartbeatFrame(std::span<const uint8_t> frame_data) {
    try {
        CdmpHeartbeatMessage heartbeat = CdmpHeartbeatMessage::FromCanFrame(frame_data);
        network_service_->UpdateDeviceFromHeartbeat(heartbeat);
        LOG_DBG("Processed heartbeat frame");
    } catch (const std::exception& e) {
        LOG_ERR("Error processing heartbeat frame: %s", e.what());
    }
}

void CdmpHeartbeatService::SendHeartbeat() {
    if(!canbus_ || !heartbeat_enabled_)
        return;

    try {
        CdmpHeartbeatMessage heartbeat{
            .device_id = device_->GetDeviceId(),
            .health_status = device_->GetHealthStatus(),
            .sequence_number = heartbeat_sequence_number_,
            .capability_flags = device_->GetCapabilityFlags()
        };

        auto frame_data = heartbeat.ToCanFrame();
        uint32_t frame_id = can_id_manager_->GetHeartbeatCanId();
        canbus_->SendFrame(frame_id, frame_data);

        last_heartbeat_sent_ = k_uptime_get();
        heartbeat_sequence_number_++;

        LOG_DBG("Sent heartbeat, sequence: %d", heartbeat_sequence_number_ - 1);
    } catch (const std::exception& e) {
        LOG_ERR("Error sending heartbeat: %s", e.what());
    }
}

void CdmpHeartbeatService::SetHeartbeatInterval(uint64_t interval) {
    if(interval < K_MSEC(100).ticks) {
        LOG_WRN("Heartbeat interval too short, minimum is 100ms");
        return;
    }

    heartbeat_interval_ = interval;
    LOG_INF("Heartbeat interval set to %lld ms", interval);
}

void CdmpHeartbeatService::SetHeartbeatEnabled(bool enabled) {
    heartbeat_enabled_ = enabled;
    LOG_INF("Heartbeat %s", enabled ? "enabled" : "disabled");

    if(enabled)
        last_heartbeat_sent_ = k_uptime_get();
}

void CdmpHeartbeatService::SendHeartbeatNow() {
    SendHeartbeat();
}

// TODO: Use work_queue to reschedule heartbeat
void CdmpHeartbeatService::ProcessPeriodicHeartbeat() {
    if(!heartbeat_enabled_) return;

    uint64_t current_time = k_uptime_get();
    if(current_time - last_heartbeat_sent_ >= heartbeat_interval_)
        SendHeartbeat();
}

void CdmpHeartbeatService::PrintHeartbeatStatus() const {
    LOG_INF("Heartbeat Status:");
    LOG_INF("  Enabled: %s", heartbeat_enabled_ ? "Yes" : "No");
    LOG_INF("  Interval: %lld ms", heartbeat_interval_);
    LOG_INF("  Last Sent: %lld ms ago",
           heartbeat_enabled_ ? (k_uptime_get() - last_heartbeat_sent_) : 0);
    LOG_INF("  Sequence Number: %d", heartbeat_sequence_number_);
}

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
