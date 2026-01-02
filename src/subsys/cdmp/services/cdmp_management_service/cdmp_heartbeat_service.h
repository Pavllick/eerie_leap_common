#pragma once

#include "../../models/cdmp_message.h"

#include "../cdmp_canbus_service_base.h"

#include <zephyr/kernel.h>

namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

class CdmpHeartbeatService : public CdmpCanbusServiceBase {
private:
    // Heartbeat configuration
    uint64_t heartbeat_interval_;
    uint64_t last_heartbeat_sent_;
    uint8_t heartbeat_sequence_number_;
    bool heartbeat_enabled_;

    // CAN handlers for heartbeat-related messages
    int heartbeat_handler_id_;

    void SendHeartbeat();

public:
    CdmpHeartbeatService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device,
        std::shared_ptr<CdmpStatusMachine> status_machine);

    ~CdmpHeartbeatService();

    void Start() override;
    void Stop() override;

    void ProcessFrame(std::span<const uint8_t> frame_data);

    // Heartbeat management
    void SetHeartbeatInterval(uint64_t interval);
    uint64_t GetHeartbeatInterval() const { return heartbeat_interval_; }
    void SetHeartbeatEnabled(bool enabled);
    bool IsHeartbeatEnabled() const { return heartbeat_enabled_; }

    // Heartbeat operations
    void SendHeartbeatNow();
    void ProcessPeriodicHeartbeat();

    // Heartbeat status
    uint64_t GetLastHeartbeatTime() const { return last_heartbeat_sent_; }
    uint8_t GetHeartbeatSequenceNumber() const { return heartbeat_sequence_number_; }

    // Diagnostics
    void PrintHeartbeatStatus() const;

    // Constants
    static constexpr uint64_t DEFAULT_HEARTBEAT_INTERVAL = K_SECONDS(3).ticks;
};

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
