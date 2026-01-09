#pragma once

#include <cstdint>
#include <vector>
#include <span>

#include "canbus_com_command_code.h"

namespace eerie_leap::domain::canbus_com_domain::commands {

class ICanbusComCommand {
public:
    virtual ~ICanbusComCommand() = default;

    virtual CanbusComCommandCode GetCommandCode() const = 0;
    // NOTE: Maximum size of the data is 5 bytes
    // for Classical 8 bytes CAN frame
    virtual std::vector<uint8_t> GetData() const = 0;
};

} // namespace eerie_leap::domain::canbus_com_domain::commands
