#pragma once

#include <functional>
#include <memory>

#include <zephyr/kernel.h>

#include "work_queue_task_result.h"

namespace eerie_leap::subsys::threading {

// Type-erased base class
class WorkQueueTaskBase {
public:
    k_work_q* work_q;
    k_work_delayable work;

    virtual ~WorkQueueTaskBase() = default;
    virtual WorkQueueTaskResult Execute() = 0;
};

// Templated derived class that stores the actual type
template<typename T>
class WorkQueueTask : public WorkQueueTaskBase {
private:
    std::unique_ptr<T> user_data_;
    T* user_data_ptr_;

public:
    using Handler = std::function<WorkQueueTaskResult(T*)>;

    Handler handler;

    void SetUserData(T* user_data) {
        user_data_ptr_ = user_data;
    }

    void SetUserData(std::unique_ptr<T> user_data) {
        user_data_ = std::move(user_data);
        SetUserData(user_data_.get());
    }

    T* GetUserdata() const {
        return user_data_ptr_;
    }

    WorkQueueTaskResult Execute() override {
        return handler(user_data_ptr_);
    }
};

class WorkQueueRunnerTask : public WorkQueueTaskBase {
private:
    std::vector<std::unique_ptr<WorkQueueRunnerTask>>& runner_tasks_;

public:
    using Handler = std::function<void()>;

    explicit WorkQueueRunnerTask(std::vector<std::unique_ptr<WorkQueueRunnerTask>>& runner_tasks)
        : runner_tasks_(runner_tasks) {}

    Handler handler;

    WorkQueueTaskResult Execute() override {
        handler();

        return {
            .reschedule = false
        };
    }

    std::vector<std::unique_ptr<WorkQueueRunnerTask>>& GetRunnerTasks() {
        return runner_tasks_;
    }
};

} // namespace eerie_leap::subsys::threading
