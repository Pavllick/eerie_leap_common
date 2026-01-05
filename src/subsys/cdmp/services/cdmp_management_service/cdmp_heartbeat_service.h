#pragma once

#include <span>
#include <optional>

#include <zephyr/kernel.h>

#include "subsys/threading/work_queue_thread.h"

#include "../cdmp_canbus_service_base.h"
#include "cdmp_network_service.h"
namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

using namespace eerie_leap::subsys::threading;

class CdmpHeartbeatService : public CdmpCanbusServiceBase {
private:
    std::shared_ptr<WorkQueueThread> work_queue_thread_;
    std::shared_ptr<CdmpNetworkService> network_service_;

    std::optional<WorkQueueTask<CdmpHeartbeatService>> heartbeat_task_;
    bool is_heartbeat_task_running_ = false;
    static WorkQueueTaskResult ProcessPeriodicHeartbeat(CdmpHeartbeatService* instance);

    // Heartbeat configuration
    uint8_t heartbeat_sequence_number_ = 0;
    bool heartbeat_enabled_ = true;

    void OnDeviceStatusChanged(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) override;

    void StartHeartbeatTask();
    void SendHeartbeat();
    void ProcessHeartbeatFrame(std::span<const uint8_t> frame_data);

public:
    CdmpHeartbeatService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device,
        std::shared_ptr<WorkQueueThread> work_queue_thread,
        std::shared_ptr<CdmpNetworkService> network_service);

    ~CdmpHeartbeatService();

    void Start() override;
    void Stop() override;

    void ProcessFrame(uint32_t frame_id, std::span<const uint8_t> frame_data) override;

    // Heartbeat management
    void SetHeartbeatEnabled(bool enabled);
    bool IsHeartbeatEnabled() const { return heartbeat_enabled_; }

    // Diagnostics
    void PrintHeartbeatStatus() const;
};

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
