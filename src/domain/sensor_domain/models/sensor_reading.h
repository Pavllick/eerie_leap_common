#pragma once

#include <memory_resource>
#include <optional>
#include <string>
#include <chrono>

#include "utilities/guid/guid.hpp"
#include "sensor.h"
#include "reading_source.h"
#include "reading_status.h"
#include "reading_metadata.h"

namespace eerie_leap::domain::sensor_domain::models {

using namespace std::chrono;
using namespace eerie_leap::utilities::guid;

struct SensorReading {
    using allocator_type = std::pmr::polymorphic_allocator<>;

    const Guid id;
    const std::shared_ptr<Sensor> sensor;
    std::optional<float> value;
    std::optional<system_clock::time_point> timestamp;
    ReadingSource source = ReadingSource::NONE;
    ReadingStatus status = ReadingStatus::UNINITIALIZED;
    std::optional<std::pmr::string> error_message;
    ReadingMetadata metadata;

    SensorReading(
        std::allocator_arg_t,
        allocator_type alloc,
        const Guid id,
        std::shared_ptr<Sensor> sensor)
            : id(id),
                sensor(std::move(sensor)),
                metadata(std::allocator_arg, alloc) {}

    SensorReading(const SensorReading& other) = default;
	SensorReading& operator=(const SensorReading&) = delete;
	SensorReading& operator=(SensorReading&&) = delete;
	SensorReading(SensorReading&&) noexcept = default;
	~SensorReading() = default;

    SensorReading(SensorReading&& other, allocator_type alloc) noexcept
        : id(other.id),
        sensor(other.sensor),
        value(other.value),
        timestamp(other.timestamp),
        source(other.source),
        status(other.status),
        error_message(other.error_message),
        metadata(std::move(other.metadata), alloc) {}
};

}

// namespace eerie_leap::domain::sensor_domain::models
