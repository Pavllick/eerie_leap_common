#pragma once

#include <cstdint>

namespace eerie_leap::domain::sensor_domain::models {

enum class SensorReadingUpdateMethod : uint8_t {
    NONE,
    ISR,
    SCHEDULER,
};

}

// namespace eerie_leap::domain::sensor_domain::models
