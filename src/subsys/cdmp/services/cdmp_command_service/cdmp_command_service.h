#pragma once

#include <functional>
#include <vector>
#include <unordered_map>

#include "../../models/cdmp_message.h"
#include "../cdmp_canbus_service_base.h"

namespace eerie_leap::subsys::cdmp::services::cdmp_command_service {

using CommandHandler = std::function<void(const CdmpCommandMessage&, uint8_t transaction_id)>;

class CdmpCommandService : public CdmpCanbusServiceBase {
private:
    int canbus_handler_id_;
    int canbus_response_handler_id_;

    std::unordered_map<CdmpCommandCode, CommandHandler> command_handlers_;
    // TODO: Implement transaction handling, add CdmpTransactionService

    void RegisterCanHandlers();
    void UnregisterCanHandlers();

    void ProcessFrame(std::span<const uint8_t> frame_data);
    void ProcessResponseFrame(std::span<const uint8_t> frame_data);
    void SendCommandResponse(const CdmpCommandResponse& response);

public:
    CdmpCommandService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device,
        std::shared_ptr<CdmpStatusMachine> status_machine);

    ~CdmpCommandService();

    void Start() override;
    void Stop() override;

    void RegisterCommandHandler(CdmpCommandCode command_code, CommandHandler handler);
    void UnregisterCommandHandler(CdmpCommandCode command_code);

    uint8_t SendCommand(
        uint8_t target_device_id,
        CdmpCommandCode command_code,
        const std::vector<uint8_t>& data = {});
    bool SendCommandAndWaitForResponse(
        uint8_t target_device_id,
        CdmpCommandCode command_code,
        const std::vector<uint8_t>& data,
        CdmpCommandResponse& response,
        uint64_t timeout = K_SECONDS(1).ticks);
};

} // namespace eerie_leap::subsys::cdmp::services::cdmp_command_service
