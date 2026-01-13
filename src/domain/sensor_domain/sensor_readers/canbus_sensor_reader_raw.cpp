#include <stdexcept>

#include "canbus_sensor_reader_raw.h"

namespace eerie_leap::domain::sensor_domain::sensor_readers {

using namespace eerie_leap::subsys::canbus;

CanbusSensorReaderRaw::CanbusSensorReaderRaw(
    std::shared_ptr<ITimeService> time_service,
    std::shared_ptr<GuidGenerator> guid_generator,
    std::shared_ptr<SensorReadingsFrame> sensor_readings_frame,
    std::shared_ptr<Sensor> sensor,
    std::shared_ptr<Canbus> canbus)
        : SensorReaderBase(
            std::move(time_service),
            std::move(guid_generator),
            std::move(sensor_readings_frame),
            std::move(sensor)),
        canbus_(std::move(canbus)) {

    uint32_t frame_id = sensor_->configuration.canbus_source->frame_id;

    int handler_id = canbus_->RegisterFrameReceivedHandler(
        frame_id,
        [this](const CanFrame& frame) {
            // AddOrUpdateReading(frame);
        });

    if(handler_id < 0)
        throw std::runtime_error("Failed to register CAN frame handler for frame ID: " + std::to_string(frame_id));

    if(!registered_handler_ids_.contains(frame_id))
        registered_handler_ids_.insert({frame_id, {}});
    registered_handler_ids_[frame_id].push_back(handler_id);
}

CanbusSensorReaderRaw::~CanbusSensorReaderRaw() {
    for(const auto& [frame_id, handler_ids] : registered_handler_ids_)
        for(const auto& handler_id : handler_ids)
            canbus_->RemoveFrameReceivedHandler(frame_id, handler_id);
}

std::optional<SensorReading> CanbusSensorReaderRaw::CreateRawReading(const CanFrame& can_frame) {
    if(can_frame.data.empty())
        return std::nullopt;

    SensorReading reading(std::allocator_arg, Mrm::GetExtPmr(), guid_generator_->Generate(), sensor_);
    reading.source = ReadingSource::ISR;
    reading.timestamp = time_service_->GetCurrentTime();

    reading.value = std::nullopt;
    reading.status = ReadingStatus::RAW;

    reading.metadata.AddTag<CanFrame>(
        ReadingMetadataTag::CANBUS_DATA,
        can_frame);

    return reading;
}

void CanbusSensorReaderRaw::AddOrUpdateReading(const CanFrame can_frame) {
    auto reading = CreateRawReading(can_frame);
    if(!reading)
        return;

    sensor_readings_frame_->AddOrUpdateReading(reading.value());
}

} // namespace eerie_leap::domain::sensor_domain::sensor_readers
