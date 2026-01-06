#pragma once

#include "subsys/threading/work_queue_thread.h"

#include "../cdmp_canbus_service_base.h"
#include "cdmp_network_service.h"
#include "cdmp_heartbeat_service.h"

namespace eerie_leap::subsys::cdmp::services::cdmp_management_service {

using namespace eerie_leap::subsys::threading;

class CdmpManagementService : public CdmpCanbusServiceBase {
private:
    std::shared_ptr<ITimeService> time_service_;
    std::shared_ptr<WorkQueueThread> work_queue_thread_;
    int canbus_handler_id_ = -1;

    std::vector<std::shared_ptr<ICdmpCanbusService>> canbus_services_;

    void RegisterCanHandlers();
    void UnregisterCanHandlers();

public:
    CdmpManagementService(
        std::shared_ptr<Canbus> canbus,
        std::shared_ptr<CdmpCanIdManager> can_id_manager,
        std::shared_ptr<CdmpDevice> device,
        std::shared_ptr<ITimeService> time_service,
        std::shared_ptr<WorkQueueThread> work_queue_thread);

    ~CdmpManagementService();

    void Initialize() override;
    void Start() override;
    void Stop() override;

    void ProcessFrame(uint32_t frame_id, std::span<const uint8_t> frame_data) override;
};

} // namespace eerie_leap::subsys::cdmp::services::cdmp_management_service
