#pragma once

#include "../cdmp_canbus_service_base.h"
#include "cdmp_network_service.h"
#include "cdmp_heartbeat_service.h"

namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

class CdmpManagementService : public CdmpCanbusServiceBase {
private:
    int canbus_handler_id_ = -1;

    std::shared_ptr<CdmpNetworkService> network_service_;
    std::unique_ptr<CdmpHeartbeatService> heartbeat_service_;

    void SendDiscoveryRequest();
    void SendManagementMessage(const CdmpManagementMessage& msg);

    void RegisterCanHandlers();
    void UnregisterCanHandlers();

public:
    CdmpManagementService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device);

    ~CdmpManagementService();

    void Start() override;
    void Stop() override;

    void ProcessFrame(std::span<const uint8_t> frame_data);
};

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
