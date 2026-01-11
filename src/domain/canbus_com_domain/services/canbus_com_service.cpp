#include <utility>

#include "subsys/random/rng.h"

#include "canbus_com_service.h"

namespace eerie_leap::domain::canbus_com_domain::services {

using namespace eerie_leap::subsys::random;

CanbusComService::CanbusComService(std::shared_ptr<CanbusService> canbus_service)
    : canbus_service_(std::move(canbus_service)) {

    cdmp_service_ = std::make_shared<CdmpService>(
        canbus_service_->GetCanbus(0),  // TODO: Make configurable
        static_cast<CdmpDeviceType>(CONFIG_EERIE_LEAP_DOMAIN_CANBUS_COM_DEVICE_TYPE),
        Rng::Get32(true));
}

void CanbusComService::Initialize() {
    cdmp_service_->Initialize();
}

void CanbusComService::Start() {
    cdmp_service_->Start();
}

void CanbusComService::UnsetCommandHandler(CanbusComCommandCode command_code) {
    cdmp_service_->GetCommandService()->UnregisterCommandHandler(
        std::to_underlying(command_code));
}

void CanbusComService::SendCommand(ICanbusComCommand& command, CommandAckCallback callback, uint8_t device_id) {
    if(callback) {
        cdmp_service_->GetCommandService()->SendCommand(
            device_id,
            std::to_underlying(command.GetCommandCode()),
            command.GetData(),
            [callback](uint8_t _, const CdmpResultCode result_code, std::span<const uint8_t> data) {
                callback(result_code == CdmpResultCode::SUCCESS);
            });
    } else {
        cdmp_service_->GetCommandService()->SendCommand(
            device_id,
            std::to_underlying(command.GetCommandCode()),
            command.GetData());
    }
}

} // namespace eerie_leap::domain::canbus_com_domain::services
