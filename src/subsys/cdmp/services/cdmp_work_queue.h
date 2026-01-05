#pragma once

#include <memory>
#include <functional>

#include <zephyr/kernel.h>

#include "subsys/threading/work_queue_thread.h"

namespace eerie_leap::subsys::cdmp::services {

using namespace eerie_leap::subsys::threading;

class CdmpWorkQueue {
private:
    static constexpr int thread_stack_size_ = 4096;
    static constexpr int thread_priority_ = 5;
    std::unique_ptr<WorkQueueThread> work_queue_thread_;

public:
    CdmpWorkQueue();
    ~CdmpWorkQueue() = default;

    void Initialize();
    WorkQueueThread* GetWorkQueue();

    void Run(const std::function<void()>& func) {
        work_queue_thread_->Run(func);
    }
};

} // namespace eerie_leap::subsys::cdmp::services
