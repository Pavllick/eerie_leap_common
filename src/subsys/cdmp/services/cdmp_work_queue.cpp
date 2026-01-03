#include "cdmp_work_queue.h"

namespace eerie_leap::subsys::cdmp::services {

CdmpWorkQueue::CdmpWorkQueue()
    : work_queue_thread_(nullptr) {}

void CdmpWorkQueue::Initialize() {
    work_queue_thread_ = std::make_unique<WorkQueueThread>(
        "cdmp_work_queue",
        thread_stack_size_,
        thread_priority_);

    work_queue_thread_->Initialize();
}

WorkQueueThread* CdmpWorkQueue::GetWorkQueue() {
    if(!work_queue_thread_)
        return nullptr;

    return work_queue_thread_.get();
}

} // namespace eerie_leap::subsys::cdmp::services
