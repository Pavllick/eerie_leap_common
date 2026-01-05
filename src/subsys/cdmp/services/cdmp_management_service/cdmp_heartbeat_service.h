#pragma once

#include <span>

#include <zephyr/kernel.h>

#include "../cdmp_canbus_service_base.h"
#include "cdmp_network_service.h"
namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

class CdmpHeartbeatService : public CdmpCanbusServiceBase {
private:
    std::shared_ptr<CdmpNetworkService> network_service_;

    // Heartbeat configuration
    uint64_t heartbeat_interval_ = DEFAULT_HEARTBEAT_INTERVAL;
    uint64_t last_heartbeat_sent_ = 0;
    uint8_t heartbeat_sequence_number_ = 0;
    bool heartbeat_enabled_ = true;

    void SendHeartbeat();
    void ProcessHeartbeatFrame(std::span<const uint8_t> frame_data);

public:
    CdmpHeartbeatService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device,
        std::shared_ptr<CdmpNetworkService> network_service);

    ~CdmpHeartbeatService();

    void Start() override;
    void Stop() override;

    void ProcessFrame(uint32_t frame_id, std::span<const uint8_t> frame_data) override;

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
