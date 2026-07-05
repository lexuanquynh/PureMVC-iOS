//
//  ThreadExecutor.hpp
//  PureMVC Core — Infrastructure
//
//  Simple IExecutor that runs each task on its own detached thread. Adequate as
//  a default; a bounded thread pool can replace it behind the same interface.
//

#ifndef PUREMVC_CORE_THREAD_EXECUTOR_HPP
#define PUREMVC_CORE_THREAD_EXECUTOR_HPP

#include <thread>
#include <utility>
#include "Domain/Ports/IExecutor.hpp"

namespace core {

class ThreadExecutor : public IExecutor {
public:
    void run(Task task) override {
        std::thread(std::move(task)).detach();
    }
};

} // namespace core

#endif // PUREMVC_CORE_THREAD_EXECUTOR_HPP
