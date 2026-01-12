#include <memory>
#include <stdexcept>

#include "collect_isr_reading_processor.h"

namespace eerie_leap::domain::sensor_domain::processors {

CollectIsrReadingProcessor::CollectIsrReadingProcessor(std::shared_ptr<SensorReadingsFrame> sensor_readings_frame) :
    sensor_readings_frame_(std::move(sensor_readings_frame)) {}

void CollectIsrReadingProcessor::ProcessReading(const size_t sensor_id_hash) {
    if(!sensor_readings_frame_->HasIsrReading(sensor_id_hash))
        return;

    auto reading = sensor_readings_frame_->GetIsrReading(sensor_id_hash);

    try {
        if(reading.status != ReadingStatus::RAW)
            throw std::invalid_argument("Reading is in wrong state");

        reading.source = ReadingSource::PROCESSING;

        sensor_readings_frame_->AddOrUpdateReading(reading);
    } catch (const std::exception& e) {
        reading.status = ReadingStatus::ERROR;
        reading.error_message = e.what();
    }
}

} // namespace eerie_leap::domain::sensor_domain::processors
