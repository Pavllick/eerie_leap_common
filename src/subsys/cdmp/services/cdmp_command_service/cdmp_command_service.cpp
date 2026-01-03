#include <zephyr/logging/log.h>

#include "cdmp_command_service.h"

LOG_MODULE_REGISTER(cdmp_command_service, LOG_LEVEL_INF);

namespace eerie_leap::subsys::cdmp::services::cdmp_command_service {

CdmpCommandService::CdmpCommandService(
    std::shared_ptr<Canbus> canbus,
    std::shared_ptr<CdmpCanIdManager> can_id_manager,
    std::shared_ptr<CdmpDevice> device)
    : CdmpCanbusServiceBase(std::move(canbus), std::move(can_id_manager), std::move(device))
    , canbus_handler_id_(-1)
    , canbus_response_handler_id_(-1) {
}

CdmpCommandService::~CdmpCommandService() {
    Stop();
}

void CdmpCommandService::Start() {
    RegisterCanHandlers();
    LOG_INF("CDMP Command Service started");
}

void CdmpCommandService::Stop() {
    UnregisterCanHandlers();
    LOG_INF("CDMP Command Service stopped");
}

void CdmpCommandService::RegisterCanHandlers() {
    if (!canbus_) return;

    canbus_handler_id_ = canbus_->RegisterFrameReceivedHandler(
        can_id_manager_->GetCommandCanId(),
        [this](const CanFrame& frame) { ProcessFrame(frame.data); });

    canbus_response_handler_id_ = canbus_->RegisterFrameReceivedHandler(
        can_id_manager_->GetCommandResponseCanId(),
        [this](const CanFrame& frame) { ProcessResponseFrame(frame.data); });
}

void CdmpCommandService::UnregisterCanHandlers() {
    if (!canbus_) return;

    if (canbus_handler_id_ >= 0) {
        canbus_->RemoveFrameReceivedHandler(can_id_manager_->GetCommandCanId(), canbus_handler_id_);
        canbus_handler_id_ = -1;
    }

    if (canbus_response_handler_id_ >= 0) {
        canbus_->RemoveFrameReceivedHandler(can_id_manager_->GetCommandResponseCanId(), canbus_response_handler_id_);
        canbus_response_handler_id_ = -1;
    }
}

void CdmpCommandService::ProcessFrame(std::span<const uint8_t> frame_data) {
    try {
        CdmpCommandMessage command = CdmpCommandMessage::FromCanFrame(frame_data);

        if(command.target_device_id != device_->GetDeviceId()
            && command.target_device_id != CdmpDevice::DEVICE_ID_BROADCAST) {

            return;
        }

        // if (device_ && device_->GetStatus() == CdmpDeviceStatus::ONLINE) {
        //     // Handle normal commands
        // } else if (command.command_code == CdmpCommandCode::STATUS_REQUEST) {
        //     // Status request is allowed even when not fully online
        // }

        // LOG_INF("Received command 0x%02X for device %d, transaction %d",
        //     static_cast<uint8_t>(command.command_code), command.target_device_id, command.transaction_id);


        auto handler_it = command_handlers_.find(command.command_code);
        if (handler_it != command_handlers_.end()) {
            handler_it->second(command, command.transaction_id);
            LOG_DBG("Processed command %d for transaction %d",
                   std::to_underlying(command.command_code), command.transaction_id);
        } else {
            LOG_WRN("No handler registered for command %d", std::to_underlying(command.command_code));

            // Send error response
            CdmpCommandResponse response{
                .source_device_id = device_->GetDeviceId(),
                // .target_device_id = command.source_device_id,
                .command_code = command.command_code,
                .transaction_id = command.transaction_id,
                .result_code = CdmpResultCode::UNSUPPORTED_COMMAND,
                .data = {}
            };
            SendCommandResponse(response);
        }
    } catch (const std::exception& e) {
        LOG_ERR("Error processing command frame: %s", e.what());
    }
}

void CdmpCommandService::ProcessResponseFrame(std::span<const uint8_t> frame_data) {
    try {
        CdmpCommandResponse response = CdmpCommandResponse::FromCanFrame(frame_data);
        LOG_DBG("Received command response for transaction %d, code %d",
               response.transaction_id, std::to_underlying(response.result_code));

        // Response handling would be delegated to transaction service
    } catch (const std::exception& e) {
        LOG_ERR("Error processing command response frame: %s", e.what());
    }
}

void CdmpCommandService::SendCommandResponse(const CdmpCommandResponse& response) {
    if(!canbus_)
        return;

    try {
        auto frame = response.ToCanFrame();
        uint32_t can_id = can_id_manager_->GetCommandResponseCanId();
        canbus_->SendFrame(can_id, frame);

        LOG_DBG("Sent command response for transaction %d", response.transaction_id);
    } catch (const std::exception& e) {
        LOG_ERR("Error sending command response: %s", e.what());
    }
}

void CdmpCommandService::RegisterCommandHandler(CdmpCommandCode command_code, CommandHandler handler) {
    command_handlers_[command_code] = std::move(handler);
    LOG_DBG("Registered handler for command %d", std::to_underlying(command_code));
}

void CdmpCommandService::UnregisterCommandHandler(CdmpCommandCode command_code) {
    command_handlers_.erase(command_code);
    LOG_DBG("Unregistered handler for command %d", std::to_underlying(command_code));
}

uint8_t CdmpCommandService::SendCommand(
    uint8_t target_device_id,
    CdmpCommandCode command_code,
    const std::vector<uint8_t>& data) {

    if(!canbus_)
        return 0;

    try {
        // Transaction ID generation would be delegated to transaction service
        uint8_t transaction_id = 1; // Placeholder

        CdmpCommandMessage command{
            // .source_device_id = device_->GetDeviceId(),
            .target_device_id = target_device_id,
            .command_code = command_code,
            .transaction_id = transaction_id,
            .data = data
        };

        auto frame_data = command.ToCanFrame();
        uint32_t frame_id = can_id_manager_->GetCommandCanId();
        canbus_->SendFrame(frame_id, frame_data);

        LOG_DBG("Sent command %d to device %d, transaction %d",
            std::to_underlying(command_code), target_device_id, transaction_id);

        return transaction_id;
    } catch (const std::exception& e) {
        LOG_ERR("Error sending command: %s", e.what());
        return 0;
    }
}

bool CdmpCommandService::SendCommandAndWaitForResponse(
    uint8_t target_device_id,
    CdmpCommandCode command_code,
    const std::vector<uint8_t>& data,
    CdmpCommandResponse& response,
    uint64_t timeout) {

    // This would integrate with transaction service for proper wait/response handling
    uint8_t transaction_id = SendCommand(target_device_id, command_code, data);
    return transaction_id != 0;
}

} // namespace eerie_leap::subsys::cdmp::services::cdmp_command_service
