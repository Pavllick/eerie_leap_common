#pragma once

#include <cstdint>
#include <span>

namespace eerie_leap::subsys::cdmp::services {

class ICdmpCanbusService {
public:
    virtual ~ICdmpCanbusService() = default;

    virtual void Initialize() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;

    virtual void ProcessFrame(uint32_t frame_id, std::span<const uint8_t> frame_data) = 0;
};

} // namespace eerie_leap::subsys::cdmp::services
