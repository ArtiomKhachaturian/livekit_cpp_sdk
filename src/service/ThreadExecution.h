// Copyright 2025 Artiom Khachaturian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once
#include "Loggable.h"
#include "ThreadPriority.h"
#include "ProtectedObj.h"
#include <atomic>
#include <string>
#include <system_error>
#include <thread>

namespace LiveKitCpp
{

class ThreadExecution : public SharedLoggerLoggable<>
{
public:
    virtual ~ThreadExecution();
    const std::string& GetThreadName() const noexcept { return _threadName; }
    ThreadPriority GetPriority() const noexcept { return _priority; }
    void startExecution(bool waitingUntilNotStarted = false);
    // don't forget to call this method before destroy of derived class instance
    void stopExecution();
    // maybe started but not active until thread is not yet really started
    bool started() const noexcept;
    bool active() const noexcept;
protected:
    ThreadExecution(std::string threadName = std::string(),
                    ThreadPriority priority = ThreadPriority::High,
                    const std::shared_ptr<LogsReceiver>& logger = {});
    // called inside of thread routine, after start
    virtual void doExecuteInThread() = 0;
    // called after changes of internal state to 'stopped' but before joining of execution thread
    virtual void doStopThread() {}
    // called if setup ot thread priority or name was failed,
    // can be ignored because it's not critical issues
    virtual void onSetThreadPriorityError(const std::error_code& error);
    virtual void onSetThreadNameError(const std::error_code& /*error*/) {}
private:
    ThreadExecution(const ThreadExecution&) = delete;
    ThreadExecution(ThreadExecution&&) = delete;
    void joinAndDestroyThread();
    void execute();
    std::error_code SetCurrentThreadName() const;
    std::error_code SetCurrentThreadPriority() const;
private:
    const std::string _threadName;
    const ThreadPriority _priority;
    ProtectedObj<std::thread, std::mutex> _thread;
    bool _started = false;
};

} // namespace LiveKitCpp
