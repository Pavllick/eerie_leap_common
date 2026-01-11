#pragma once

#include <memory_resource>
#include <unordered_map>
#include <optional>

#include "can_channel_configuration.h"

namespace eerie_leap::domain::canbus_domain::models {

struct CanbusConfiguration {
    using allocator_type = std::pmr::polymorphic_allocator<>;

    std::pmr::unordered_map<uint8_t, CanChannelConfiguration> channel_configurations;
    std::optional<uint8_t> com_bus_channel = std::nullopt;

    CanbusConfiguration(std::allocator_arg_t, allocator_type alloc)
        : channel_configurations(alloc) {}

    CanbusConfiguration(const CanbusConfiguration&) = delete;
	CanbusConfiguration& operator=(const CanbusConfiguration&) noexcept = default;
	CanbusConfiguration& operator=(CanbusConfiguration&&) noexcept = default;
	CanbusConfiguration(CanbusConfiguration&&) noexcept = default;
	~CanbusConfiguration() = default;

    CanbusConfiguration(CanbusConfiguration&& other, allocator_type alloc)
        : channel_configurations(std::move(other.channel_configurations), alloc),
          com_bus_channel(other.com_bus_channel) {}
};

} // namespace eerie_leap::domain::canbus_domain::models
