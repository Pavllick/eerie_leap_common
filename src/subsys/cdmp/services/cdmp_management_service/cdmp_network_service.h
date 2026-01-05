#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <span>
#include <optional>

#include "subsys/threading/work_queue_thread.h"

#include "../cdmp_canbus_service_base.h"
#include "../cdmp_work_queue.h"


namespace eerie_leap::subsys::cdmp::services {

using namespace eerie_leap::subsys::threading;

class CdmpNetworkService : public CdmpCanbusServiceBase {
private:
    std::shared_ptr<CdmpWorkQueue> work_queue_;

    std::unordered_map<uint8_t, std::unique_ptr<CdmpDevice>> network_devices_;
    uint8_t lowest_id_on_network_ = 0;

    // Discovery and tracking
    bool auto_discovery_enabled_ = true;
    bool discovery_response_received_ = false;
    uint8_t claiming_device_id_ = 0;
    std::optional<CdmpIdClaimResult> id_claim_result_ = std::nullopt;

    static constexpr int DISCOVERY_TIMEOUT_MS = 200;
    static constexpr int ID_CLAIM_RESPONSE_TIMEOUT_MS = 50;
    static constexpr int ID_CLAIM_RETRY_COUNT = 10;

    void OnDeviceStatusChanged(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) override;

    void ProcessDiscoveryResponseFrame(std::span<const uint8_t> frame_data);
    void SendDiscoveryRequest();
    void SendDiscoveryResponse();

    void SendIdClaim();
    void ProcessIdClaimRequestFrame(std::span<const uint8_t> frame_data);
    void ProcessIdClaimResponseFrame(std::span<const uint8_t> frame_data);

    void AddOrUpdateDevice(uint8_t device_id, CdmpDeviceType device_type, uint32_t unique_identifier);
    void UpdateDeviceFromDiscovery(const CdmpDiscoveryResponseMessage& discovery);

    void RemoveOfflineDevices();
    uint8_t GetLowestAvailableId(uint8_t after = 0) const;

public:
    CdmpNetworkService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device,
        std::shared_ptr<CdmpWorkQueue> work_queue);

    ~CdmpNetworkService();

    void Start() override;
    void Stop() override;

    void ProcessFrame(uint32_t frame_id, std::span<const uint8_t> frame_data) override;
    void StartInitialization();

    // Network device management
    void UpdateDeviceFromHeartbeat(const CdmpHeartbeatMessage& heartbeat);
    void RemoveDevice(uint8_t device_id);
    void ClearAllDevices();

    // Device queries
    std::vector<uint8_t> GetOnlineDeviceIds() const;
    std::vector<uint8_t> GetAllDeviceIds() const;
    size_t GetDeviceCount() const { return network_devices_.size(); }
    const CdmpDevice* GetDevice(uint8_t device_id) const;
    CdmpDevice* GetDevice(uint8_t device_id); // Non-const version for modifications

    // Discovery management
    void SetAutoDiscovery(bool enabled);
    bool IsAutoDiscoveryEnabled() const { return auto_discovery_enabled_; }
    void ProcessPeriodicTasks();

    // Network status
    bool IsDeviceOnline(uint8_t device_id) const;
    std::vector<uint8_t> GetOfflineDeviceIds() const;

    // Diagnostics
    void PrintNetworkStatus() const;
};

} // namespace eerie_leap::subsys::cdmp::services
