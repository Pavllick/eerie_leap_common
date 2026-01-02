#pragma once

#include "../cdmp_canbus_service_base.h"
#include "cdmp_heartbeat_service.h"

namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

class CdmpManagementService : public CdmpCanbusServiceBase {
private:
    int canbus_handler_id_;

    std::unique_ptr<CdmpHeartbeatService> heartbeat_service_;

    void SendDiscoveryResponse();
    void SendDiscoveryRequest();
    void SendManagementMessage(const CdmpManagementMessage& msg);

    void RegisterCanHandlers();
    void UnregisterCanHandlers();
    void OnDeviceStatusChanged(CdmpDeviceStatus old_state, CdmpDeviceStatus new_state) override;

public:
    CdmpManagementService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device,
        std::shared_ptr<CdmpStatusMachine> status_machine);

    ~CdmpManagementService();

    void Start() override;
    void Stop() override;

    void ProcessFrame(const CanFrame& frame);
    void SendIdClaim();
};

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
