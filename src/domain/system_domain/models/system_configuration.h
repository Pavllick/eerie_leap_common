#pragma once

#include <memory_resource>
#include <cstdint>
#include <string>

namespace eerie_leap::domain::system_domain::models {

struct SystemConfiguration {
    using allocator_type = std::pmr::polymorphic_allocator<>;

    uint64_t device_id = 0;
    uint32_t hw_version = 0;
    uint32_t sw_version = 0;
    uint32_t build_number = 0;

    SystemConfiguration(std::allocator_arg_t, allocator_type alloc) {}

    SystemConfiguration(const SystemConfiguration&) = delete;
    SystemConfiguration& operator=(const SystemConfiguration&) = delete;

    SystemConfiguration(SystemConfiguration&&) noexcept = default;

    SystemConfiguration(SystemConfiguration&& other, allocator_type alloc)
        : device_id(other.device_id),
        hw_version(other.hw_version),
        sw_version(other.sw_version),
        build_number(other.build_number) {}

    std::string GetFormattedHwVersion() const {
        uint8_t hw_version_major = (hw_version >> 24) & 0xFF;
        uint8_t hw_version_minor = (hw_version >> 16) & 0xFF;
        uint16_t hw_version_patch = hw_version & 0xFFFF;

        char version_str[32];
        snprintf(version_str, sizeof(version_str), "%u.%u.%u",
            hw_version_major, hw_version_minor, hw_version_patch);

        return {version_str};
    }

    std::string GetFormattedSwVersion() const {
        uint8_t sw_version_major = (sw_version >> 24) & 0xFF;
        uint8_t sw_version_minor = (sw_version >> 16) & 0xFF;
        uint16_t sw_version_patch = sw_version & 0xFFFF;

        char version_str[32];
        snprintf(version_str, sizeof(version_str), "%u.%u.%u.%u",
            sw_version_major, sw_version_minor, sw_version_patch, build_number);

        return {version_str};
    }

    [[nodiscard]] static constexpr uint32_t GetConfigHwVersion() {
        return CONFIG_EERIE_LEAP_HW_VERSION_MAJOR << 24 |
               CONFIG_EERIE_LEAP_HW_VERSION_MINOR << 16 |
               CONFIG_EERIE_LEAP_HW_VERSION_PATCH;
    }

    [[nodiscard]] static constexpr uint32_t GetConfigSwVersion() {
        return CONFIG_EERIE_LEAP_SW_VERSION_MAJOR << 24 |
               CONFIG_EERIE_LEAP_SW_VERSION_MINOR << 16 |
               CONFIG_EERIE_LEAP_SW_VERSION_PATCH;
    }
};

} // namespace eerie_leap::domain::system_domain::models
