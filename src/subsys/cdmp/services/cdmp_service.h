#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <zephyr/kernel.h>

#include "subsys/canbus/canbus.h"
#include <subsys/threading/thread.h>
#include "subsys/threading/work_queue_thread.h"

#include "subsys/cdmp/utilities/cdmp_can_id_manager.h"
#include "subsys/cdmp/models/cdmp_device.h"
#include "subsys/cdmp/models/cdmp_message.h"

#include "i_cdmp_canbus_service.h"
#include "cdmp_service.h"

namespace eerie_leap::subsys::cdmp::services {

using namespace eerie_leap::subsys::canbus;
using namespace eerie_leap::subsys::threading;
using namespace eerie_leap::subsys::cdmp::models;
using namespace eerie_leap::subsys::cdmp::utilities;

using CommandHandler = std::function<void(const CdmpCommandMessage&, uint8_t transaction_id)>;
using CapabilityDataGenerator = std::function<std::vector<uint8_t>()>;

class CdmpService : public IThread {
private:
    static constexpr int k_stack_size_ = 4096;
    static constexpr int k_priority_ = 5;
    std::unique_ptr<Thread> thread_;

    static constexpr int work_queue_stack_size_ = 4096;
    static constexpr int work_queue_priority_ = 5;
    std::shared_ptr<WorkQueueThread> work_queue_thread_;

    std::shared_ptr<ITimeService> time_service_;
    std::shared_ptr<CdmpDevice> device_;
    std::shared_ptr<Canbus> canbus_;
    std::shared_ptr<CdmpCanIdManager> can_id_manager_;

    std::vector<std::shared_ptr<ICdmpCanbusService>> canbus_services_;

    // Device registry for tracking other devices on network
    std::unordered_map<uint8_t, std::unique_ptr<CdmpDevice>> network_devices_;

    // Transaction management
    uint8_t next_transaction_id_ = 1;
    std::unordered_map<uint8_t, uint64_t> pending_transactions_;
    static constexpr uint64_t TRANSACTION_TIMEOUT = K_MSEC(500).ticks;

    // Configuration
    bool is_running_ = false;
    uint32_t base_can_id_;
    bool auto_discovery_enabled_ = true;

    // Timing
    uint64_t last_status_broadcast_ = 0;
    uint64_t status_broadcast_interval_ = DEFAULT_STATUS_BROADCAST_INTERVAL;
    uint8_t status_sequence_number_ = 0;

    void ThreadEntry() override;

    // Transaction management
    uint8_t GetNextTransactionId();
    void StartTransaction(uint8_t transaction_id);
    void CompleteTransaction(uint8_t transaction_id);
    void CleanupExpiredTransactions();

    // Work queue tasks
    static WorkQueueTaskResult ProcessCdmpTask(void* task_data);
    void ProcessPeriodicTasks();
    void ProcessStateTimeouts();
    void ProcessTransactionTimeouts();
    void ProcessHeartbeatTimeouts();

public:
    CdmpService(
        std::shared_ptr<ITimeService> time_service,
        std::shared_ptr<Canbus> canbus,
        CdmpDeviceType device_type,
        uint32_t uid,
        uint32_t base_can_id = CdmpCanIdManager::DEFAULT_BASE_CAN_ID);
    ~CdmpService();

    // Service lifecycle
    bool Initialize();
    void Start();
    void Stop();
    bool IsRunning() const;

    // Configuration
    void SetAutoDiscovery(bool enabled);
    void SetDeviceType(CdmpDeviceType device_type);

    // Direct command sending
    uint8_t SendCommand(
        uint8_t target_device_id,
        CdmpCommandCode command_code,
        const std::vector<uint8_t>& parameters = {});
    bool SendCommandAndWaitForResponse(
        uint8_t target_device_id,
        CdmpCommandCode command_code,
        const std::vector<uint8_t>& parameters,
        CdmpCommandResponse& response,
        uint64_t timeout = K_SECONDS(1).ticks);

    // Configuration management
    bool ReadRemoteConfig(uint8_t target_device_id,
        CdmpConfigType config_type,
        std::vector<uint8_t>& config_data);
    bool WriteRemoteConfig(uint8_t target_device_id,
        CdmpConfigType config_type,
        const std::vector<uint8_t>& config_data);
    uint32_t GetRemoteConfigCrc(uint8_t target_device_id, CdmpConfigType config_type);

    // Network information
    CdmpDevice& GetDevice() const;
    const CdmpDevice* GetDevice(uint8_t device_id) const;
    std::vector<uint8_t> GetOnlineDeviceIds() const;
    size_t GetDeviceCount() const { return network_devices_.size(); }

    // Diagnostics
    void PrintNetworkStatus() const;
    void PrintDeviceStatus() const;

    // Constants
    static constexpr uint64_t DEFAULT_HEARTBEAT_INTERVAL = K_SECONDS(3).ticks;
    static constexpr uint64_t DEFAULT_STATUS_BROADCAST_INTERVAL = K_SECONDS(1).ticks;
};

} // namespace eerie_leap::subsys::cdmp::services
