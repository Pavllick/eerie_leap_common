#pragma once

namespace eerie_leap::domain::sensor_domain::services {

class ISensorsProcessingService {
public:
    virtual ~ISensorsProcessingService() = default;

    virtual void Initialize() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
};

} // namespace eerie_leap::domain::sensor_domain::services
