#pragma once

namespace eerie_leap::subsys::threading {

class IThread {
public:
    virtual ~IThread() = default;
    virtual void ThreadEntry() = 0;
};

} // namespace eerie_leap::subsys::threading
