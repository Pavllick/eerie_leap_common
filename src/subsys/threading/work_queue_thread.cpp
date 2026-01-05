#include <ranges>
#include <algorithm>

#include "work_queue_thread.h"

namespace eerie_leap::subsys::threading {

WorkQueueThread::WorkQueueThread(std::string name, int stack_size, int priority, bool is_cooperative)
    : ThreadBase(std::move(name), stack_size, priority, is_cooperative) {}

WorkQueueThread::~WorkQueueThread() {
    k_work_queue_stop(&work_q_, K_FOREVER);
}

void WorkQueueThread::Initialize() {
    InitializeStack();

    k_work_queue_init(&work_q_);
    k_work_queue_start(&work_q_, stack_area_, k_stack_size_, k_priority_, nullptr);

    k_thread_name_set(&work_q_.thread, name_.c_str());
}

[[nodiscard]] k_work_q* WorkQueueThread::GetWorkQueue() {
    return &work_q_;
}

void WorkQueueThread::ScheduleTask(WorkQueueTaskBase& task) {
    k_work_schedule_for_queue(&work_q_, &task.work, K_NO_WAIT);
}

void WorkQueueThread::TaskHandler(k_work* work) {
    WorkQueueTaskBase* task = CONTAINER_OF(work, WorkQueueTaskBase, work);
    auto result = task->Execute();

    if(result.reschedule)
        k_work_reschedule_for_queue(task->work_q, &task->work, result.delay);
}

void WorkQueueThread::RunnerTaskHandler(k_work* work) {
    WorkQueueRunnerTask* task = CONTAINER_OF(work, WorkQueueRunnerTask, work);
    auto result = task->Execute();

    auto& runner_tasks = task->GetRunnerTasks();
    auto it = std::ranges::find_if(runner_tasks, [task](const std::unique_ptr<WorkQueueRunnerTask>& obj) {
        return obj->work_q == task->work_q && &obj->work == &task->work; });

    if(it != runner_tasks.end())
        runner_tasks.erase(it);
}

bool WorkQueueThread::CancelTask(WorkQueueTaskBase& task) {
    return k_work_cancel_delayable_sync(&task.work, &sync_);
}

bool WorkQueueThread::FlushTask(WorkQueueTaskBase& task) {
    return k_work_flush_delayable(&task.work, &sync_);
}

void WorkQueueThread::Run(const WorkQueueRunnerTask::Handler& handler) {
    auto task = std::make_unique<WorkQueueRunnerTask>(runner_tasks_);
    task->work_q = &work_q_;
    task->handler = handler;

    k_work_init_delayable(&task->work, RunnerTaskHandler);
    ScheduleTask(*task);
    runner_tasks_.push_back(std::move(task));
}

} // namespace eerie_leap::subsys::threading
