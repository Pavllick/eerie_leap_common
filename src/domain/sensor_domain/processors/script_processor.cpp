#include "script_processor.h"

namespace eerie_leap::domain::sensor_domain::processors {

ScriptProcessor::ScriptProcessor(const std::string& function_name, std::shared_ptr<SensorReadingsFrame> sensor_readings_frame)
    : function_name_(function_name), sensor_readings_frame_(std::move(sensor_readings_frame)) {}

void ScriptProcessor::ProcessReading(const size_t sensor_id_hash) {
    if(!sensor_readings_frame_->HasReading(sensor_id_hash))
        return;

    auto reading = sensor_readings_frame_->GetReading(sensor_id_hash);

    try {
        auto lua_script = reading.sensor->configuration.lua_script;
        if(lua_script == nullptr)
            return;

        auto* state = lua_script->GetState();
        if(state == nullptr)
            return;

        lua_getglobal(state, function_name_.c_str());

        if(!lua_isfunction(state, -1)) {
            lua_pop(state, 1);
            return;
        }

        lua_pushstring(state, reading.sensor->id.c_str());

        if(lua_pcall(state, 1, 0, 0) != LUA_OK)
            lua_pop(state, 1);
    } catch (const std::exception& e) {
        reading.status = ReadingStatus::ERROR;
        reading.error_message = e.what();
    }
}

} // namespace eerie_leap::domain::sensor_domain::processors
