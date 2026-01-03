#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <functional>

#include <zephyr/kernel.h>

namespace eerie_leap::subsys::cdmp::utilities {

enum class CdmpDeviceStatus : uint8_t {
    OFFLINE = 0x00,
    INIT = 0x01,
    CLAIMING = 0x02,
    ONLINE = 0x03,
    VERSION_MISMATCH = 0x04,
    ERROR = 0x05
};

class CdmpStatusMachine {
public:
    using StatusChangeCallback = std::function<void(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status)>;

private:
    CdmpDeviceStatus status_;
    std::unordered_map<int, StatusChangeCallback> status_change_callbacks_;
    int64_t state_timeout_ = 0;
    uint8_t retry_count_ = 0;
    static constexpr uint8_t MAX_RETRIES = 3;

    static constexpr uint64_t DISCOVERY_TIMEOUT_MS = K_MSEC(200).ticks;
    static constexpr uint64_t ID_CLAIM_RESPONSE_TIMEOUT_MS = K_MSEC(50).ticks;
    static constexpr uint64_t COMMAND_TIMEOUT_MS = K_MSEC(100).ticks;
    // static constexpr uint64_t HEARTBEAT_TIMEOUT_MS = K_SECONDS(9).ticks; // 3x heartbeat interval

    static constexpr uint64_t RETRY_BACKOFF_1_MS = K_MSEC(100).ticks;
    static constexpr uint64_t RETRY_BACKOFF_2_MS = K_MSEC(200).ticks;
    static constexpr uint64_t RETRY_BACKOFF_3_MS = K_MSEC(500).ticks;

    void NotifyStatusChange(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status);
    bool IsValidTransition(CdmpDeviceStatus from, CdmpDeviceStatus to) const;

public:
    CdmpStatusMachine(CdmpDeviceStatus initial_status);

    // Status management
    void SetStatus(CdmpDeviceStatus new_status, bool force = false);
    CdmpDeviceStatus GetCurrentStatus() const;

    // Callbacks
    int RegisterStatusChangeHandler(StatusChangeCallback callback);
    void UnregisterStatusChangeHandler(int callback_id);

    // Validation
    bool CanTransitionTo(CdmpDeviceStatus new_status) const;
    bool IsOnline() const { return GetCurrentStatus() == CdmpDeviceStatus::ONLINE; }
    bool IsOperational() const;
};

} // namespace eerie_leap::subsys::cdmp::utilities
