#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <span>

#include "../cdmp_canbus_service_base.h"

namespace eerie_leap::subsys::cdmp::services {

class CdmpNetworkService : public CdmpCanbusServiceBase {
private:
    // Network device registry
    std::unordered_map<uint8_t, std::unique_ptr<CdmpDevice>> network_devices_;

    // Discovery and tracking
    bool auto_discovery_enabled_ = true;
    uint64_t last_discovery_scan_ = 0;
    static constexpr uint64_t DISCOVERY_SCAN_INTERVAL = K_SECONDS(10).ticks;

    void OnDeviceStatusChanged(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) override;

    void ProcessDiscoveryFrame(std::span<const uint8_t> frame_data);
    void SendDiscoveryRequest();
    void SendDiscoveryResponse();
    void SendIdClaim();

    void AddOrUpdateDevice(uint8_t device_id, CdmpDeviceType device_type, uint32_t unique_identifier);
    void UpdateDeviceFromDiscovery(const CdmpDiscoveryResponseMessage& discovery);

    void RemoveOfflineDevices();

public:
    CdmpNetworkService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device);

    ~CdmpNetworkService();

    void Start() override;
    void Stop() override;

    void ProcessFrame(std::span<const uint8_t> frame_data);

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
