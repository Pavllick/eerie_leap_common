#include <ranges>
#include <algorithm>

#include "work_queue_thread.h"

namespace eerie_leap::subsys::threading {

WorkQueueThread::WorkQueueThread(std::string name, int stack_size, int priority, bool is_cooperative)
    : ThreadBase(std::move(name), stack_size, priority, is_cooperative) {}

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

    auto& runner_tasks = task->GetRunnerTasks();
    runner_tasks.erase(work);
}

void WorkQueueThread::Run(const WorkQueueRunnerTask::Handler& handler) {
    IsValid();

    WorkQueueRunnerTask task(
        &work_q_, &sync_, RunnerTaskHandler, handler, runner_tasks_);
    runner_tasks_.insert({&task.work.work, std::move(task)});

    runner_tasks_.at(&task.work.work).Schedule();
}

} // namespace eerie_leap::subsys::threading
