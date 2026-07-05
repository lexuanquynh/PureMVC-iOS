//
//  SyncExecutor.hpp
//  PureMVC Core tests
//
//  Runs tasks inline on the calling thread so async code becomes deterministic.
//

#ifndef PUREMVC_CORE_SYNC_EXECUTOR_HPP
#define PUREMVC_CORE_SYNC_EXECUTOR_HPP

#include "Domain/Ports/IExecutor.hpp"

namespace core { namespace test {

class SyncExecutor : public IExecutor {
public:
    int runCount = 0;

    void run(Task task) override {
        ++runCount;
        task();
    }
};

}} // namespace core::test

#endif // PUREMVC_CORE_SYNC_EXECUTOR_HPP
