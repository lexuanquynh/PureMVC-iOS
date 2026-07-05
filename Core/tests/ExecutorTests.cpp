//
//  ExecutorTests.cpp
//  PureMVC Core tests
//

#include <gtest/gtest.h>

#include <future>

#include "Infrastructure/Concurrency/ThreadExecutor.hpp"
#include "Mocks/SyncExecutor.hpp"

using namespace core;

TEST(SyncExecutor, RunsTaskInlineOnCallingThread) {
    test::SyncExecutor executor;
    bool ran = false;

    executor.run([&ran]() { ran = true; });

    EXPECT_TRUE(ran);              // already run by the time run() returns
    EXPECT_EQ(executor.runCount, 1);
}

TEST(ThreadExecutor, RunsTaskOffThreadAndCompletes) {
    ThreadExecutor executor;
    std::promise<int> promise;
    std::future<int> future = promise.get_future();

    executor.run([&promise]() { promise.set_value(42); });

    ASSERT_EQ(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    EXPECT_EQ(future.get(), 42);
}
