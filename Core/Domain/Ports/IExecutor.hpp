//
//  IExecutor.hpp
//  PureMVC Core — outbound port (concurrency)
//
//  Abstracts "run this work off the caller's thread". Production backs it with
//  a thread pool / GCD; tests use a synchronous executor for determinism.
//

#ifndef PUREMVC_CORE_IEXECUTOR_HPP
#define PUREMVC_CORE_IEXECUTOR_HPP

#include <functional>

namespace core {

class IExecutor {
public:
    using Task = std::function<void()>;

    virtual void run(Task task) = 0;

    virtual ~IExecutor() = default;
};

} // namespace core

#endif // PUREMVC_CORE_IEXECUTOR_HPP
