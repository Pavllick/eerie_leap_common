#pragma once

#include <memory>

#include "utilities/guid/guid_generator.h"
#include "subsys/time/i_time_service.h"
#include "subsys/threading/work_queue_thread.h"

#include "domain/sensor_domain/configuration/adc_configuration_manager.h"
#include "domain/sensor_domain/utilities/sensor_readings_frame.hpp"
#include "domain/sensor_domain/models/sensor.h"
#include "domain/canbus_domain/services/canbus_service.h"
#include "i_isr_sensor_reader.h"

namespace eerie_leap::domain::sensor_domain::isr_sensor_readers {

using namespace eerie_leap::utilities::guid;
using namespace eerie_leap::subsys::time;
using namespace eerie_leap::subsys::threading;

using namespace eerie_leap::domain::sensor_domain::configuration;
using namespace eerie_leap::domain::sensor_domain::utilities;
using namespace eerie_leap::domain::sensor_domain::models;
using namespace eerie_leap::domain::canbus_domain::services;

class IsrSensorReaderFactory {
protected:
    std::shared_ptr<ITimeService> time_service_;
    std::shared_ptr<GuidGenerator> guid_generator_;
    std::shared_ptr<SensorReadingsFrame> sensor_readings_frame_;
    std::shared_ptr<CanbusService> canbus_service_;

public:
    IsrSensorReaderFactory(
        std::shared_ptr<ITimeService> time_service,
        std::shared_ptr<GuidGenerator> guid_generator,
        std::shared_ptr<SensorReadingsFrame> sensor_readings_frame,
        std::shared_ptr<CanbusService> canbus_service);

    virtual ~IsrSensorReaderFactory() = default;

    std::unique_ptr<IIsrSensorReader> Create(
        std::shared_ptr<Sensor> sensor,
        std::shared_ptr<WorkQueueThread> work_queue_thread,
        ProcessSensorCallback process_sensor_callback);
};

} // namespace eerie_leap::domain::sensor_domain::isr_sensor_readers
