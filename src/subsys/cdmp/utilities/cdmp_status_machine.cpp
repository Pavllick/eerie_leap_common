#include <utility>

#include <zephyr/logging/log.h>

#include "cdmp_status_machine.h"

LOG_MODULE_REGISTER(cdmp_status_machine, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::utilities {

CdmpStatusMachine::CdmpStatusMachine(std::shared_ptr<CdmpDevice> device)
    : device_(std::move(device)), state_timeout_(0), retry_count_(0) {}

void CdmpStatusMachine::SetStatus(CdmpDeviceStatus new_state) {
    CdmpDeviceStatus old_state = device_->GetStatus();

    if(!CanTransitionTo(new_state)) {
        LOG_WRN("Invalid state transition from %d to %d",
            std::to_underlying(old_state), std::to_underlying(new_state));
        return;
    }

    device_->SetStatus(new_state);
    StopStatusTimeout();
    ResetRetryCount();

    LOG_INF("Status transition: %d -> %d",
        std::to_underlying(old_state), std::to_underlying(new_state));
    NotifyStatusChange(old_state, new_state);
}

CdmpDeviceStatus CdmpStatusMachine::GetCurrentStatus() const {
    return device_->GetStatus();
}

void CdmpStatusMachine::Initialize() {
    StartDiscovery();
}

void CdmpStatusMachine::StartDiscovery() {
    SetStatus(CdmpDeviceStatus::INIT);
    StartStatusTimeout(DISCOVERY_TIMEOUT_MS);
    LOG_INF("Starting device discovery");
}

void CdmpStatusMachine::ClaimId(uint8_t device_id) {
    device_->SetDeviceId(device_id);
    SetStatus(CdmpDeviceStatus::CLAIMING);
    StartStatusTimeout(ID_CLAIM_RESPONSE_TIMEOUT_MS);
    LOG_INF("Claiming device ID: %d", device_id);
}

void CdmpStatusMachine::GoOnline() {
    SetStatus(CdmpDeviceStatus::ONLINE);
    LOG_INF("Device is now online with ID: %d", device_ ? device_->GetDeviceId() : 0);
}

void CdmpStatusMachine::EnterVersionMismatch() {
    SetStatus(CdmpDeviceStatus::VERSION_MISMATCH);
    LOG_ERR("Protocol version mismatch detected");
}

void CdmpStatusMachine::EnterError() {
    SetStatus(CdmpDeviceStatus::ERROR);
    LOG_ERR("Device entered error state");
}

void CdmpStatusMachine::Reset() {
    SetStatus(CdmpDeviceStatus::OFFLINE);

    device_->SetDeviceId(0);
    device_->SetHealthStatus(CdmpHealthStatus::OK);

    LOG_INF("Device reset to offline state");
}

void CdmpStatusMachine::StartStatusTimeout(uint64_t timeout) {
    state_timeout_ = k_uptime_get() + timeout;
}

void CdmpStatusMachine::StopStatusTimeout() {
    state_timeout_ = 0;
}

bool CdmpStatusMachine::IsStatusTimedOut() const {
    if(state_timeout_ == 0)
        return false;

    return k_uptime_get() > state_timeout_;
}

// TODO: Add call by timer
void CdmpStatusMachine::HandleStatusTimeout() {
    CdmpDeviceStatus current_state = GetCurrentStatus();

    LOG_WRN("Status timeout in state %d, retry count: %d",
        std::to_underlying(current_state), retry_count_);

    switch (current_state) {
        case CdmpDeviceStatus::INIT:
            if(ShouldRetry()) {
                IncrementRetryCount();
                StartDiscovery();
                k_sleep(K_TIMEOUT_ABS_MS(GetRetryBackoff()));
            } else {
                // No responses received, assume empty network, claim ID 1
                ClaimId(1);
            }
            break;

        case CdmpDeviceStatus::CLAIMING:
            if(ShouldRetry()) {
                IncrementRetryCount();
                // Retry with different ID or restart discovery
                StartDiscovery();
                k_sleep(K_TIMEOUT_ABS_MS(GetRetryBackoff()));
            } else {
                EnterError();
            }
            break;

        default:
            LOG_WRN("Timeout in unexpected state: %d", static_cast<int>(current_state));
            break;
    }
}

void CdmpStatusMachine::IncrementRetryCount() {
    retry_count_++;
}

void CdmpStatusMachine::ResetRetryCount() {
    retry_count_ = 0;
}

uint64_t CdmpStatusMachine::GetRetryBackoff() const {
    switch(retry_count_) {
        case 1: return RETRY_BACKOFF_1_MS;
        case 2: return RETRY_BACKOFF_2_MS;
        case 3: return RETRY_BACKOFF_3_MS;
        default: return K_MSEC(100).ticks;
    }
}

void CdmpStatusMachine::OnDiscoveryResponseReceived() {
    if(GetCurrentStatus() == CdmpDeviceStatus::INIT) {
        // Discovery responses received, proceed with ID selection
        ResetRetryCount();
        LOG_DBG("Discovery response received");
    }
}

void CdmpStatusMachine::OnIdClaimResponseReceived(CdmpIdClaimResult result) {
    if(GetCurrentStatus() != CdmpDeviceStatus::CLAIMING)
        return;

    switch(result) {
        case CdmpIdClaimResult::ACCEPT:
            GoOnline();
            break;

        case CdmpIdClaimResult::REJECT:
            if(ShouldRetry()) {
                IncrementRetryCount();
                StartDiscovery();
                k_sleep(K_TIMEOUT_ABS_MS(GetRetryBackoff()));
            } else {
                EnterError();
            }
            break;

        case CdmpIdClaimResult::VERSION_INCOMPATIBLE:
            EnterVersionMismatch();
            break;
    }
}

void CdmpStatusMachine::OnVersionMismatchDetected() {
    EnterVersionMismatch();
}

void CdmpStatusMachine::OnHeartbeatReceived() {
    if(device_) {
        device_->UpdateHeartbeat();
    }
}

void CdmpStatusMachine::OnHeartbeatTimeout() {
    if(GetCurrentStatus() == CdmpDeviceStatus::ONLINE) {
        LOG_WRN("Heartbeat timeout detected");
        EnterError();
    }
}

void CdmpStatusMachine::OnCommandReceived() {
    // Commands can be received in ONLINE state
    if(GetCurrentStatus() == CdmpDeviceStatus::ONLINE)
        StartStatusTimeout(COMMAND_TIMEOUT_MS);
}

void CdmpStatusMachine::OnErrorDetected() {
    EnterError();
}

int CdmpStatusMachine::RegisterStatusChangeHandler(StatusChangeCallback callback) {
        int max_id = 0;
        for(const auto& [id, _] : status_change_callbacks_) {
            if(id > max_id)
                max_id = id;
        }
        int new_id = max_id + 1;

        status_change_callbacks_[new_id] = callback;
        return new_id;
    }

void CdmpStatusMachine::UnregisterStatusChangeHandler(int callback_id) {
    status_change_callbacks_.erase(callback_id);
}

bool CdmpStatusMachine::CanTransitionTo(CdmpDeviceStatus new_state) const {
    auto current_state = GetCurrentStatus();

    return IsValidTransition(current_state, new_state);
}

bool CdmpStatusMachine::IsOperational() const {
    auto current_state = GetCurrentStatus();

    return current_state == CdmpDeviceStatus::ONLINE
        || current_state == CdmpDeviceStatus::CLAIMING;
}

void CdmpStatusMachine::NotifyStatusChange(CdmpDeviceStatus old_state, CdmpDeviceStatus new_state) {
    for(const auto& [_, callback] : status_change_callbacks_)
        callback(old_state, new_state);
}

bool CdmpStatusMachine::IsValidTransition(CdmpDeviceStatus from, CdmpDeviceStatus to) const {
    // Define valid state transitions
    switch(from) {
        case CdmpDeviceStatus::OFFLINE:
            return to == CdmpDeviceStatus::INIT;

        case CdmpDeviceStatus::INIT:
            return to == CdmpDeviceStatus::CLAIMING ||
                   to == CdmpDeviceStatus::OFFLINE;

        case CdmpDeviceStatus::CLAIMING:
            return to == CdmpDeviceStatus::ONLINE ||
                   to == CdmpDeviceStatus::VERSION_MISMATCH ||
                   to == CdmpDeviceStatus::ERROR ||
                   to == CdmpDeviceStatus::INIT;

        case CdmpDeviceStatus::ONLINE:
            return to == CdmpDeviceStatus::VERSION_MISMATCH ||
                   to == CdmpDeviceStatus::ERROR ||
                   to == CdmpDeviceStatus::OFFLINE;

        case CdmpDeviceStatus::VERSION_MISMATCH:
            return to == CdmpDeviceStatus::OFFLINE;

        case CdmpDeviceStatus::ERROR:
            return to == CdmpDeviceStatus::INIT ||
                   to == CdmpDeviceStatus::OFFLINE;

        default:
            return false;
    }
}

} // namespace eerie_leap::subsys::cdmp::utilities
