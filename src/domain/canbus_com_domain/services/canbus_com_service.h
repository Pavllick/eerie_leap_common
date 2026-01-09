#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <concepts>
#include <optional>

#include "utilities/concepts/concepts.h"

#include "subsys/cdmp/utilities/constants.h"
#include "subsys/cdmp/services/cdmp_service.h"

#include "domain/canbus_domain/services/canbus_service.h"
#include "domain/canbus_com_domain/commands/i_canbus_com_command.h"

namespace eerie_leap::domain::canbus_com_domain::services {

using namespace eerie_leap::utilities::concepts;

using namespace eerie_leap::subsys::cdmp::utilities;
using namespace eerie_leap::subsys::cdmp::services;

using namespace eerie_leap::domain::canbus_domain::services;
using namespace eerie_leap::domain::canbus_com_domain::commands;

class CanbusComService {
private:
    std::shared_ptr<CanbusService> canbus_service_;
    std::shared_ptr<CdmpService> cdmp_service_;

public:
    using CommandAckCallback = std::function<void(bool success)>;
    template<SpanConstructible TResponse>
    using CommandDataResponseCallback = std::function<void(bool success, std::optional<TResponse> response)>;

    template<SpanConstructible TRequest>
    using CommandDataRequestCallback = std::function<void(std::optional<TRequest> request)>;

    CanbusComService(std::shared_ptr<CanbusService> canbus_service);
    virtual ~CanbusComService() = default;

    void Initialize();
    void Start();

    template<SpanConstructible TRequest>
    void SetCommandHandler(CanbusComCommandCode command_code, CommandDataRequestCallback<TRequest> callback) {
        cdmp_service_->GetCommandService()->RegisterUserCommandHandler(
            std::to_underlying(command_code),
            [callback](uint8_t _, std::span<const uint8_t> data) {
                callback(data.empty() ? std::nullopt : std::optional<TRequest>(data));
        });
    }

    void UnsetCommandHandler(CanbusComCommandCode command_code);

    void SendCommand(
        ICanbusComCommand& command,
        CommandAckCallback callback = nullptr,
        uint8_t device_id = CdmpConstants::COMMAND_BROADCAST_ID);

    template<SpanConstructible TResponse>
    void SendCommand(
        ICanbusComCommand& command,
        CommandDataResponseCallback<TResponse> callback,
        uint8_t device_id = CdmpConstants::COMMAND_BROADCAST_ID) {

        SendCommand(command, [callback](uint8_t _, const CdmpResultCode result_code, std::span<const uint8_t> data) {
            callback(
                result_code == CdmpResultCode::SUCCESS,
                result_code == CdmpResultCode::SUCCESS ? std::optional<TResponse>(data) : std::nullopt);
        }, device_id);
    }
};

} // namespace eerie_leap::domain::canbus_com_domain::services
