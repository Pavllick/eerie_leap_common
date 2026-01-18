#include <cmath>

#include "thread_base.h"

namespace eerie_leap::subsys::threading {

ThreadBase::ThreadBase(std::string name, int stack_size, int priority, bool is_cooperative)
    : k_stack_size_(stack_size),
      k_priority_(is_cooperative
        ? std::abs(priority) * -1 : std::abs(priority)),
      stack_area_(nullptr),
      name_(std::move(name)) {}

ThreadBase::~ThreadBase() {
    if(stack_area_ == nullptr)
        return;

    k_thread_stack_free(stack_area_);
}

void ThreadBase::InitializeStack() {
    stack_area_ = k_thread_stack_alloc(k_stack_size_, 0);
}

[[nodiscard]] const k_thread_stack_t* ThreadBase::GetStack() const {
    return stack_area_;
}

} // namespace eerie_leap::subsys::threading
