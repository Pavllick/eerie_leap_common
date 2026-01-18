#pragma once

#include <unordered_map>
#include <stdexcept>
#include <memory_resource>
#include <string>

#include <zephyr/spinlock.h>

#include "utilities/string/string_helpers.h"
#include "domain/sensor_domain/models/sensor_reading.h"

namespace eerie_leap::domain::sensor_domain::utilities {

using namespace eerie_leap::utilities::string;
using namespace eerie_leap::domain::sensor_domain::models;

class SensorReadingsFrame {
public:
    using allocator_type = std::pmr::polymorphic_allocator<>;

private:
    std::pmr::unordered_map<size_t, SensorReading> isr_readings_;
    std::pmr::unordered_map<size_t, SensorReading> readings_;
    std::pmr::unordered_map<size_t, SensorReading> processed_readings_;
    std::unordered_map<size_t, float> reading_values_;
    mutable std::unordered_map<std::string, size_t> sensor_id_hash_map_;

    mutable k_sem processing_semaphore_;
    allocator_type allocator_;

    size_t GetSensorIdHash(const std::string& sensor_id) const {
        if(!sensor_id_hash_map_.contains(sensor_id))
            sensor_id_hash_map_.emplace(sensor_id, StringHelpers::GetHash(sensor_id));

        return sensor_id_hash_map_.at(sensor_id);
    }

    static inline void ErrorSensorIdNotFound() {
        throw std::runtime_error("Sensor ID not found");
    }

    void AddOrUpdateReadingIsr(SensorReading& reading) {
        size_t sensor_id_hash = reading.sensor->id_hash;

        if(isr_readings_.contains(sensor_id_hash))
            isr_readings_.erase(sensor_id_hash);
        isr_readings_.insert({ sensor_id_hash, reading });

        if(reading.status == ReadingStatus::PROCESSED && reading.value.has_value()) {
            reading_values_[sensor_id_hash] = reading.value.value();

            if(processed_readings_.contains(sensor_id_hash))
                processed_readings_.erase(sensor_id_hash);
            processed_readings_.insert({ sensor_id_hash, reading });
        }
    }

    void AddOrUpdateReadingProcessing(SensorReading& reading) {
        size_t sensor_id_hash = reading.sensor->id_hash;

        if(isr_readings_.contains(sensor_id_hash))
            isr_readings_.erase(sensor_id_hash);

        if(readings_.contains(sensor_id_hash))
            readings_.erase(sensor_id_hash);
        readings_.insert({ sensor_id_hash, reading });

        if(reading.status == ReadingStatus::PROCESSED && reading.value.has_value()) {
            reading_values_[sensor_id_hash] = reading.value.value();

            if(processed_readings_.contains(sensor_id_hash))
                processed_readings_.erase(sensor_id_hash);

            processed_readings_.insert({ sensor_id_hash, reading });
        }
    }

public:
    SensorReadingsFrame(std::allocator_arg_t, allocator_type alloc)
        : isr_readings_(alloc), readings_(alloc), processed_readings_(alloc), allocator_(alloc) {

        k_sem_init(&processing_semaphore_, 1, 1);
    }

    SensorReadingsFrame(const SensorReadingsFrame&) = delete;
    SensorReadingsFrame(SensorReadingsFrame&&) = delete;
    SensorReadingsFrame& operator=(const SensorReadingsFrame&) = delete;
    SensorReadingsFrame& operator=(SensorReadingsFrame&&) = delete;

    void AddOrUpdateReading(SensorReading reading) {
        k_sem_take(&processing_semaphore_, K_FOREVER);

        if(reading.source == ReadingSource::ISR)
            AddOrUpdateReadingIsr(reading);
        else if(reading.source == ReadingSource::PROCESSING)
            AddOrUpdateReadingProcessing(reading);
        else
            throw std::runtime_error("Invalid reading source");

        k_sem_give(&processing_semaphore_);
    }

    bool HasIsrReading(const size_t sensor_id_hash) const {
        return isr_readings_.contains(sensor_id_hash);
    }

    bool HasIsrReading(const std::string& sensor_id) const {
        return HasIsrReading(GetSensorIdHash(sensor_id));
    }

    bool HasReading(const size_t sensor_id_hash) const {
        return readings_.contains(sensor_id_hash);
    }

    bool HasReading(const std::string& sensor_id) const {
        return HasReading(GetSensorIdHash(sensor_id));
    }

    bool HasProcessedReading(const size_t sensor_id_hash) const {
        return processed_readings_.contains(sensor_id_hash);
    }

    bool HasProcessedReading(const std::string& sensor_id) const {
        return HasProcessedReading(GetSensorIdHash(sensor_id));
    }

    bool HasReadingValue(const size_t sensor_id_hash) const {
        return reading_values_.contains(sensor_id_hash);
    }

    bool HasReadingValue(const std::string& sensor_id) const {
        return reading_values_.contains(GetSensorIdHash(sensor_id));
    }

    SensorReading GetIsrReading(const size_t sensor_id_hash) const {
        if(!HasIsrReading(sensor_id_hash))
            ErrorSensorIdNotFound();

        return isr_readings_.at(sensor_id_hash);
    }

    SensorReading GetIsrReading(const std::string& sensor_id) const {
        const size_t sensor_id_hash = GetSensorIdHash(sensor_id);

        if(!HasIsrReading(sensor_id_hash))
            ErrorSensorIdNotFound();

        return isr_readings_.at(sensor_id_hash);
    }

    SensorReading GetReading(const size_t sensor_id_hash) const {
        if(!HasReading(sensor_id_hash))
            ErrorSensorIdNotFound();

        return readings_.at(sensor_id_hash);
    }

    SensorReading GetReading(const std::string& sensor_id) const {
        const size_t sensor_id_hash = GetSensorIdHash(sensor_id);

        if(!HasReading(sensor_id_hash))
            ErrorSensorIdNotFound();

        return readings_.at(sensor_id_hash);
    }

    SensorReading GetProcessedReading(const size_t sensor_id_hash) const {
        if(!HasProcessedReading(sensor_id_hash))
            ErrorSensorIdNotFound();

        return processed_readings_.at(sensor_id_hash);
    }

    SensorReading GetProcessedReading(const std::string& sensor_id) const {
        const size_t sensor_id_hash = GetSensorIdHash(sensor_id);

        if(!HasProcessedReading(sensor_id_hash))
            ErrorSensorIdNotFound();

        return processed_readings_.at(sensor_id_hash);
    }

    float GetReadingValue(const size_t sensor_id_hash) const {
        if(!HasReadingValue(sensor_id_hash))
            ErrorSensorIdNotFound();

        return reading_values_.at(sensor_id_hash);
    }

    float GetReadingValue(const std::string& sensor_id) const {
        const size_t sensor_id_hash = GetSensorIdHash(sensor_id);

        if(!HasReadingValue(sensor_id_hash))
            ErrorSensorIdNotFound();

        return reading_values_.at(sensor_id_hash);
    }

    float* GetReadingValuePtr(const std::string& sensor_id) {
        const size_t sensor_id_hash = GetSensorIdHash(sensor_id);

        if(!HasReadingValue(sensor_id_hash))
            ErrorSensorIdNotFound();

        return &reading_values_.at(sensor_id_hash);
    }

    std::unordered_map<size_t, SensorReading> GetProcessedReadings() const {
        k_sem_take(&processing_semaphore_, K_FOREVER);
        std::unordered_map<size_t, SensorReading> readings(
            processed_readings_.begin(), processed_readings_.end());
        k_sem_give(&processing_semaphore_);

        return readings;
    }

    void ClearProcessedReadings() {
        k_sem_take(&processing_semaphore_, K_FOREVER);
        processed_readings_.clear();
        k_sem_give(&processing_semaphore_);
    }

    void ClearReadings() {
        k_sem_take(&processing_semaphore_, K_FOREVER);
        isr_readings_.clear();
        readings_.clear();
        processed_readings_.clear();
        reading_values_.clear();
        k_sem_give(&processing_semaphore_);
    }
};

} // eerie_leap::domain::sensor_domain::utilities
