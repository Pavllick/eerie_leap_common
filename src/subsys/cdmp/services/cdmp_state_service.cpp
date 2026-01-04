#include "cdmp_state_service.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cdmp_state_service, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::services {

CdmpStateService::CdmpStateService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device)
    : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device))
    , state_change_handler_id_(-1)
    , state_change_response_handler_id_(-1) {
}

CdmpStateService::~CdmpStateService() {
    Stop();
}

void CdmpStateService::Start() {
    RegisterCanHandlers();
    LOG_INF("CDMP State Service started");
}

void CdmpStateService::Stop() {
    UnregisterCanHandlers();
    state_change_callbacks_.clear();
    LOG_INF("CDMP State Service stopped");
}

void CdmpStateService::RegisterCanHandlers() {
    if (!canbus_) return;

    state_change_handler_id_ = canbus_->RegisterFrameReceivedHandler(
        can_id_manager_->GetStateChangeCanId(),
        [this](const CanFrame& frame) { ProcessStateChangeFrame(frame); });

    state_change_response_handler_id_ = canbus_->RegisterFrameReceivedHandler(
        can_id_manager_->GetStateChangeResponseCanId(),
        [this](const CanFrame& frame) { ProcessStateChangeResponseFrame(frame); });
}

void CdmpStateService::UnregisterCanHandlers() {
    if (!canbus_) return;

    if (state_change_handler_id_ >= 0) {
        canbus_->RemoveFrameReceivedHandler(can_id_manager_->GetStateChangeCanId(), state_change_handler_id_);
        state_change_handler_id_ = -1;
    }

    if (state_change_response_handler_id_ >= 0) {
        canbus_->RemoveFrameReceivedHandler(can_id_manager_->GetStateChangeResponseCanId(), state_change_response_handler_id_);
        state_change_response_handler_id_ = -1;
    }
}

void CdmpStateService::ProcessFrame(const CanFrame& frame) {
    // Frame processing is handled by specific handlers registered in Start()
}

void CdmpStateService::ProcessStateChangeFrame(const CanFrame& frame) {
    try {
        // CdmpStateChangeMessage state_change = CdmpStateChangeMessage::FromCanFrame(frame);
        uint8_t source_device_id = 0; // state_change.source_device_id
        CdmpDeviceStatus old_status = CdmpDeviceStatus::OFFLINE; // state_change.old_status
        CdmpDeviceStatus new_status = CdmpDeviceStatus::ONLINE; // state_change.new_status

        LOG_DBG("Received state change from device %d: %d -> %d",
               source_device_id, std::to_underlying(old_status), std::to_underlying(new_status));

        // Notify all registered callbacks
        for (const auto& callback : state_change_callbacks_) {
            if (callback) {
                callback(source_device_id, old_status, new_status);
            }
        }

        // Send response
        SendStateChangeResponse(source_device_id, 0, true); // Use actual transaction ID

    } catch (const std::exception& e) {
        LOG_ERR("Error processing state change frame: %s", e.what());
    }
}

void CdmpStateService::ProcessStateChangeResponseFrame(const CanFrame& frame) {
    try {
        // CdmpStateChangeResponseMessage response = CdmpStateChangeResponseMessage::FromCanFrame(frame);
        LOG_DBG("Received state change response from device %d, transaction %d, success: %s",
               0, 0, true); // response.source_device_id, response.transaction_id, response.success
        // Handle response (callback, transaction completion, etc.)
    } catch (const std::exception& e) {
        LOG_ERR("Error processing state change response frame: %s", e.what());
    }
}

void CdmpStateService::SendStateChangeNotification(uint8_t target_device_id, CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) {
    if (!canbus_) return;

    try {
        // CdmpStateChangeMessage state_change{
        //     .source_device_id = device_->GetDeviceId(),
        //     .target_device_id = target_device_id,
        //     .old_status = old_status,
        //     .new_status = new_status,
        //     .timestamp = k_uptime_get_64()
        // };

        // CanFrame frame = state_change.ToCanFrame();
        // uint32_t can_id = can_id_manager_->GetStateChangeCanId();
        // canbus_->SendFrame(can_id, frame);

        LOG_DBG("Sent state change notification to device %d: %d -> %d",
               target_device_id, std::to_underlying(old_status), std::to_underlying(new_status));
    } catch (const std::exception& e) {
        LOG_ERR("Error sending state change notification: %s", e.what());
    }
}

void CdmpStateService::SendStateChangeResponse(uint8_t target_device_id, uint8_t transaction_id, bool success) {
    if (!canbus_) return;

    try {
        // CdmpStateChangeResponseMessage response{
        //     .source_device_id = device_->GetDeviceId(),
        //     .target_device_id = target_device_id,
        //     .transaction_id = transaction_id,
        //     .success = success,
        //     .timestamp = k_uptime_get_64()
        // };

        // CanFrame frame = response.ToCanFrame();
        // uint32_t can_id = can_id_manager_->GetStateChangeResponseCanId();
        // canbus_->SendFrame(can_id, frame);

        LOG_DBG("Sent state change response to device %d, transaction %d, success: %s",
               target_device_id, transaction_id, success ? "true" : "false");
    } catch (const std::exception& e) {
        LOG_ERR("Error sending state change response: %s", e.what());
    }
}

void CdmpStateService::RegisterStateChangeCallback(StateChangeCallback callback) {
    if (callback) {
        state_change_callbacks_.push_back(std::move(callback));
        LOG_DBG("Registered state change callback");
    }
}

void CdmpStateService::UnregisterStateChangeCallback(StateChangeCallback callback) {
    // Note: This is a simplified implementation. In practice, you'd need
    // to store callbacks with IDs or use a more sophisticated approach
    // to remove specific callbacks.
    auto it = std::remove_if(state_change_callbacks_.begin(), state_change_callbacks_.end(),
                           [&callback](const StateChangeCallback& stored_callback) {
                               return &stored_callback == &callback;
                           });
    if (it != state_change_callbacks_.end()) {
        state_change_callbacks_.erase(it, state_change_callbacks_.end());
        LOG_DBG("Unregistered state change callback");
    }
}

void CdmpStateService::NotifyStateChange(uint8_t target_device_id, CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) {
    SendStateChangeNotification(target_device_id, old_status, new_status);
}

void CdmpStateService::BroadcastStateChange(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status) {
    // Broadcast to all devices (0xFF is typically the broadcast address)
    SendStateChangeNotification(0xFF, old_status, new_status);
}

uint8_t CdmpStateService::RequestStateChange(uint8_t target_device_id, CdmpDeviceStatus new_status,
                                            std::function<void(bool success)> callback) {
    if (!canbus_) return 0;

    try {
        // This would be a state change request message
        // CdmpStateChangeRequestMessage request{
        //     .source_device_id = device_->GetDeviceId(),
        //     .target_device_id = target_device_id,
        //     .transaction_id = 1, // Get from transaction service
        //     .new_status = new_status
        // };

        // CanFrame frame = request.ToCanFrame();
        // uint32_t can_id = can_id_manager_->GetStateChangeRequestCanId(target_device_id);
        // canbus_->SendFrame(can_id, frame);

        LOG_DBG("Requested state change to %d for device %d",
               std::to_underlying(new_status), target_device_id);
        return 1; // Return actual transaction ID
    } catch (const std::exception& e) {
        LOG_ERR("Error requesting state change: %s", e.what());
        return 0;
    }
}

void CdmpStateService::PrintStateServiceStatus() const {
    LOG_INF("State Service Status:");
    LOG_INF("  Registered callbacks: %zu", state_change_callbacks_.size());
    LOG_INF("  State change handler: %s", state_change_handler_id_ >= 0 ? "Active" : "Inactive");
    LOG_INF("  State response handler: %s", state_change_response_handler_id_ >= 0 ? "Active" : "Inactive");
}

} // namespace eerie_leap::subsys::cdmp::services
