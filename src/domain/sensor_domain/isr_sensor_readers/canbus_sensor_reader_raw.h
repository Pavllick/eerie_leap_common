#pragma once

#include <memory>
#include <chrono>
#include <vector>
#include <unordered_map>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>

#include "subsys/threading/work_queue_thread.h"
#include "subsys/canbus/canbus.h"

#include "isr_sensor_reader_base.h"

namespace eerie_leap::domain::sensor_domain::isr_sensor_readers {

using namespace std::chrono;
using namespace eerie_leap::subsys::threading;
using namespace eerie_leap::subsys::canbus;

class CanbusSensorReaderRaw : public IsrSensorReaderBase {
private:
    std::shared_ptr<WorkQueueThread> work_queue_thread_;
    std::shared_ptr<Canbus> canbus_;

    k_sem processing_semaphore_;
    std::unordered_map<uint32_t, std::vector<int>> registered_handler_ids_;

    static constexpr int FRAME_PROCESSING_DELAY_MS = 4;

protected:
    std::optional<SensorReading> CreateRawReading(const CanFrame& can_frame);
    virtual void AddOrUpdateReading(const CanFrame can_frame);

public:
    CanbusSensorReaderRaw(
        std::shared_ptr<ITimeService> time_service,
        std::shared_ptr<GuidGenerator> guid_generator,
        std::shared_ptr<SensorReadingsFrame> sensor_readings_frame,
        std::shared_ptr<Sensor> sensor,
        ProcessSensorCallback process_sensor_callback,
        std::shared_ptr<WorkQueueThread> work_queue_thread,
        std::shared_ptr<Canbus> canbus);
    virtual ~CanbusSensorReaderRaw();
};

} // namespace eerie_leap::domain::sensor_domain::isr_sensor_readers
