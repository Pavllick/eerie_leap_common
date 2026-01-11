// #pragma once

// #include "subsys/cdmp/models/cdmp_message.h"

// #include "cdmp_canbus_service_base.h"

// #include <functional>
// #include <vector>

// namespace eerie_leap::subsys::cdmp::services {

// using StateChangeCallback = std::function<void(uint8_t source_device_id, CdmpDeviceStatus old_status, CdmpDeviceStatus new_status)>;

// class CdmpStateService : public CdmpCanbusServiceBase {
// private:
//     // State change callbacks
//     std::vector<StateChangeCallback> state_change_callbacks_;

//     // CAN handlers for state-related messages
//     int state_change_handler_id_;
//     int state_change_response_handler_id_;

//     void RegisterCanHandlers();
//     void UnregisterCanHandlers();

//     void ProcessRequestFrame(const CanFrame& frame);
//     void ProcessResponseFrame(const CanFrame& frame);

//     void SendStateChangeNotification(uint8_t target_device_id, CdmpDeviceStatus old_status, CdmpDeviceStatus new_status);
//     void SendStateChangeResponse(uint8_t target_device_id, uint8_t transaction_id, bool success);

//     void RegisterStateChangeCallback(StateChangeCallback callback);
//     void UnregisterStateChangeCallback(StateChangeCallback callback);

// public:
//     CdmpStateService(
//         std::shared_ptr<Canbus> canbus,
//         std::shared_ptr<CdmpCanIdManager> can_id_manager,
//         std::shared_ptr<CdmpDevice> device);

//     ~CdmpStateService();

//     void Start() override;
//     void Stop() override;

//     // State change operations
//     void NotifyStateChange(uint8_t target_device_id, CdmpDeviceStatus old_status, CdmpDeviceStatus new_status);
//     void BroadcastStateChange(CdmpDeviceStatus old_status, CdmpDeviceStatus new_status);

//     // Remote state operations
//     uint8_t RequestStateChange(uint8_t target_device_id, CdmpDeviceStatus new_status,
//                                std::function<void(bool success)> callback = nullptr);

//     // Diagnostics
//     void PrintStateServiceStatus() const;
// };

// } // namespace eerie_leap::subsys::cdmp::services
