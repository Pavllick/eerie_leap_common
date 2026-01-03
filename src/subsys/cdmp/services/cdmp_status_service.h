#pragma once

#include "cdmp_canbus_service_base.h"

namespace eerie_leap::subsys::cdmp::services {

class CdmpStatusService : public CdmpCanbusServiceBase {
private:
    int canbus_handler_id_;

    void RegisterCanHandlers();
    void UnregisterCanHandlers();

public:
    CdmpStatusService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device);

    ~CdmpStatusService();

    void Start() override;
    void Stop() override;

    void ProcessFrame(std::span<const uint8_t> frame_data);
};

} // namespace eerie_leap::subsys::cdmp::services
