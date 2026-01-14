#include <ranges>
#include <algorithm>

#include "work_queue_thread.h"

namespace eerie_leap::subsys::threading {

WorkQueueThread::WorkQueueThread(std::string name, int stack_size, int priority, bool is_cooperative)
    : ThreadBase(std::move(name), stack_size, priority, is_cooperative) {

    k_mutex_init(&runner_tasks_mutex_);
}

WorkQueueThread::~WorkQueueThread() {
    k_work_queue_stop(&work_q_, K_FOREVER);
}

void WorkQueueThread::IsValid() const {
    if(!initialized_)
        throw std::runtime_error("WorkQueueThread is not initialized.");
}

void WorkQueueThread::Initialize() {
    InitializeStack();

    k_work_queue_init(&work_q_);
    k_work_queue_start(&work_q_, stack_area_, k_stack_size_, k_priority_, nullptr);

    k_thread_name_set(&work_q_.thread, name_.c_str());

    initialized_ = true;
}

[[nodiscard]] k_work_q* WorkQueueThread::GetWorkQueue() {
    IsValid();

    return &work_q_;
}

void WorkQueueThread::TaskHandler(k_work* work) {
    WorkQueueTaskBase* task = CONTAINER_OF(work, WorkQueueTaskBase, work);
    auto result = task->Execute();

    if(result.reschedule)
        task->Reschedule(result.delay);
}

void WorkQueueThread::RunnerTaskHandler(k_work* work) {
    WorkQueueRunnerTask* task = CONTAINER_OF(work, WorkQueueRunnerTask, work);
    auto result = task->Execute();

    auto mutex = task->GetMutex();

    k_mutex_lock(mutex, K_FOREVER);
    task->GetRunnerTasks().erase(work);
    k_mutex_unlock(mutex);
}

void WorkQueueThread::Run(const WorkQueueRunnerTask::Handler& handler) {
    IsValid();

    try {
        auto task = std::make_unique<WorkQueueRunnerTask>(
            &work_q_, &sync_, RunnerTaskHandler, handler, &runner_tasks_mutex_, runner_tasks_);
        void* work_ptr = &task->work;

        k_mutex_lock(&runner_tasks_mutex_, K_FOREVER);
        if(runner_tasks_.insert({work_ptr, std::move(task)}).second)
            runner_tasks_.at(work_ptr)->Schedule();
        k_mutex_unlock(&runner_tasks_mutex_);
    } catch (const std::exception& e) {
        printk("Exception in WorkQueueThread::Run: %s\n", e.what());
    }
}

} // namespace eerie_leap::subsys::threading
