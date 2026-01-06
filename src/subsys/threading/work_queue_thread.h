#pragma once

#include <vector>
#include <string>

#include "thread_base.h"
#include "work_queue_task.h"

namespace eerie_leap::subsys::threading {

class WorkQueueThread : public ThreadBase {
private:
    k_work_q work_q_;
    k_work_sync sync_;

    bool initialized_ = false;

    std::vector<std::unique_ptr<WorkQueueRunnerTask>> runner_tasks_;

    void IsValid() const;
    static void TaskHandler(k_work* work);
    static void RunnerTaskHandler(k_work* work);

public:
    WorkQueueThread(std::string name, int stack_size, int priority, bool is_cooperative = false);
    ~WorkQueueThread();

    void Initialize();
    [[nodiscard]] k_work_q* GetWorkQueue();

    template<typename T>
    WorkQueueTask<T> CreateTask(const WorkQueueTask<T>::Handler& handler, std::unique_ptr<T> user_data) {
        IsValid();

        WorkQueueTask<T> task(&work_q_, &sync_, TaskHandler, handler);
        task.SetUserData(std::move(user_data));

        return task;
    }

    template<typename T>
    WorkQueueTask<T> CreateTask(const WorkQueueTask<T>::Handler& handler, T* user_data) {
        IsValid();

        WorkQueueTask<T> task(&work_q_, &sync_, TaskHandler, handler);
        task.SetUserData(user_data);

        return task;
    }

    void Run(const WorkQueueRunnerTask::Handler& handler);
};

} // namespace eerie_leap::subsys::threading
