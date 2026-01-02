#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <functional>

#include <zephyr/kernel.h>

#include "../models/cdmp_device.h"
#include "../models/cdmp_message.h"

namespace eerie_leap::subsys::cdmp::utilities {

using namespace eerie_leap::subsys::cdmp::models;

class CdmpStatusMachine {
public:
    using StatusChangeCallback = std::function<void(CdmpDeviceStatus old_state, CdmpDeviceStatus new_state)>;

private:
    std::shared_ptr<CdmpDevice> device_;
    std::unordered_map<int, StatusChangeCallback> status_change_callbacks_;
    int64_t state_timeout_;
    uint8_t retry_count_;
    static constexpr uint8_t MAX_RETRIES = 3;

    static constexpr uint64_t DISCOVERY_TIMEOUT_MS = K_MSEC(200).ticks;
    static constexpr uint64_t ID_CLAIM_RESPONSE_TIMEOUT_MS = K_MSEC(50).ticks;
    static constexpr uint64_t COMMAND_TIMEOUT_MS = K_MSEC(100).ticks;
    // static constexpr uint64_t HEARTBEAT_TIMEOUT_MS = K_SECONDS(9).ticks; // 3x heartbeat interval

    static constexpr uint64_t RETRY_BACKOFF_1_MS = K_MSEC(100).ticks;
    static constexpr uint64_t RETRY_BACKOFF_2_MS = K_MSEC(200).ticks;
    static constexpr uint64_t RETRY_BACKOFF_3_MS = K_MSEC(500).ticks;

    void NotifyStatusChange(CdmpDeviceStatus old_state, CdmpDeviceStatus new_state);
    bool IsValidTransition(CdmpDeviceStatus from, CdmpDeviceStatus to) const;

public:
    CdmpStatusMachine(std::shared_ptr<CdmpDevice> device);

    // Status management
    void SetStatus(CdmpDeviceStatus new_state);
    CdmpDeviceStatus GetCurrentStatus() const;

    // Status transitions
    void Initialize();
    void StartDiscovery();
    void ClaimId(uint8_t device_id);
    void GoOnline();
    void EnterVersionMismatch();
    void EnterError();
    void Reset();

    // Timeout handling
    void StartStatusTimeout(uint64_t timeout);
    void StopStatusTimeout();
    bool IsStatusTimedOut() const;
    void HandleStatusTimeout();

    // Retry management
    void IncrementRetryCount();
    void ResetRetryCount();
    uint8_t GetRetryCount() const { return retry_count_; }
    bool ShouldRetry() const { return retry_count_ < MAX_RETRIES; }
    uint64_t GetRetryBackoff() const;

    // Event handling
    void OnDiscoveryResponseReceived();
    void OnIdClaimResponseReceived(CdmpIdClaimResult result);
    void OnVersionMismatchDetected();
    void OnHeartbeatReceived();
    void OnHeartbeatTimeout();
    void OnCommandReceived();
    void OnErrorDetected();

    // Callbacks
    int RegisterStatusChangeHandler(StatusChangeCallback callback);
    void UnregisterStatusChangeHandler(int callback_id);

    // Validation
    bool CanTransitionTo(CdmpDeviceStatus new_state) const;
    bool IsOnline() const { return GetCurrentStatus() == CdmpDeviceStatus::ONLINE; }
    bool IsOperational() const;
};

} // namespace eerie_leap::subsys::cdmp::utilities
