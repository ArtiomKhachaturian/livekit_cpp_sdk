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
#include "ThreadExecution.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#include <sys/syscall.h>
#endif
#include <algorithm> // for std::max
#endif
#include <functional>

#ifdef __APPLE__
namespace {

inline auto GetNativeQOS(LiveKitCpp::ThreadPriority priority) {
    switch (priority)
    {
        case LiveKitCpp::ThreadPriority::Realtime:
            return QOS_CLASS_USER_INTERACTIVE;
        case LiveKitCpp::ThreadPriority::Highest:
            return QOS_CLASS_USER_INITIATED;
        case LiveKitCpp::ThreadPriority::High:
            return QOS_CLASS_UTILITY;
        case LiveKitCpp::ThreadPriority::Normal:
            break;
        case LiveKitCpp::ThreadPriority::Low:
            return QOS_CLASS_BACKGROUND;
        default:
            break;
    }
    return QOS_CLASS_DEFAULT;
}

}
#endif


namespace LiveKitCpp
{

ThreadExecution::ThreadExecution(std::string threadName, ThreadPriority priority)
    : _threadName(std::move(threadName))
    , _priority(priority)
{
}

ThreadExecution::~ThreadExecution()
{
    LOCK_WRITE_PROTECTED_OBJ(_thread);
    if (_started) {
        joinAndDestroyThread();
        _started = false;
    }
}

void ThreadExecution::startExecution(bool waitingUntilNotStarted)
{
    try {
        LOCK_WRITE_PROTECTED_OBJ(_thread);
        if (!_started) {
            _thread = std::thread(std::bind(&ThreadExecution::execute, this));
            _started = true;
            if (waitingUntilNotStarted) {
                while (!_thread->joinable()) {
                    std::this_thread::yield();
                }
            }
        }
    }
    catch(const std::system_error& /*error*/) {
        // TODO: log error
    }
}

void ThreadExecution::stopExecution()
{
    LOCK_WRITE_PROTECTED_OBJ(_thread);
    if (_started) {
        doStopThread();
        joinAndDestroyThread();
        _started = false;
    }
}

bool ThreadExecution::started() const noexcept
{
    LOCK_READ_PROTECTED_OBJ(_thread);
    return _started;
}

bool ThreadExecution::active() const noexcept
{
    LOCK_READ_PROTECTED_OBJ(_thread);
    return _thread->joinable();
}

void ThreadExecution::joinAndDestroyThread()
{
    if (_thread->joinable()) {
        try {
            if (std::this_thread::get_id() != _thread->get_id()) {
                _thread->join();
            }
            else {
                _thread->detach();
            }
            _thread = std::thread();
        }
        catch(const std::system_error& /*error*/) {
            // TODO: log error
        }
    }
}

void ThreadExecution::execute()
{
    if (started()) {
        auto error = SetCurrentThreadName();
        if (error) {
            onSetThreadNameError(error);
        }
        error = SetCurrentThreadPriority();
        if (error) {
            onSetThreadPriorityError(error);
        }
        doExecuteInThread();
    }
}

void ThreadExecution::onSetThreadPriorityError(const std::error_code& /*error*/)
{
    // TODO: add error logs
}

std::error_code ThreadExecution::SetCurrentThreadName() const
{
    int error = 0;
    if (!_threadName.empty()) {
#ifdef WIN32
        std::string_view nameView(_threadName);
        // Win32 has limitation for thread name - max 63 symbols
        if (nameView.size() > 62U) {
            nameView = nameView.substr(0, 62U);
        }
        struct
        {
            DWORD dwType;
            LPCSTR szName;
            DWORD dwThreadID;
            DWORD dwFlags;
        } threadname_info = {0x1000, nameView.data(), static_cast<DWORD>(-1), 0};

        __try {
            ::RaiseException(0x406D1388, 0, sizeof(threadname_info) / sizeof(DWORD), reinterpret_cast<ULONG_PTR*>(&threadname_info));
        } __except (EXCEPTION_EXECUTE_HANDLER) { /* NOLINT */ }
        return {};
#elif defined(__APPLE__)
        error = pthread_setname_np(_threadName.data());
#else
        error = prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(_threadName.data())); // NOLINT
#endif
    }
    return std::error_code(error, std::system_category()); // ok if name is not specified or empty
}

std::error_code ThreadExecution::SetCurrentThreadPriority() const
{
    int error = 0;
    if (ThreadPriority::Auto != _priority) {
#ifdef WIN32
        if (!::SetThreadPriority(::GetCurrentThread(), static_cast<int>(_priority))) {
            error = ::GetLastError();
        }
#else
        const int policy = SCHED_FIFO;
        const int min = sched_get_priority_min(policy);
        const int max = sched_get_priority_max(policy);
        if (-1 != min && -1 != max) {
            if (max - min > 2) {
                // convert ThreadPriority priority to POSIX priorities:
                sched_param param;
                const int top = max - 1;
                const int low = min + 1;
                switch (_priority) {
                    case ThreadPriority::Low:
                        param.sched_priority = low;
                        break;
                    case ThreadPriority::Normal:
                        // the -1 ensures that the High is always greater or equal to Normal
                        param.sched_priority = (low + top - 1) / 2;
                        break;
                    case ThreadPriority::High:
                        param.sched_priority = std::max(top - 2, low);
                        break;
                    case ThreadPriority::Highest:
                        param.sched_priority = std::max(top - 1, low);
                        break;
                    case ThreadPriority::Realtime:
                        param.sched_priority = top;
                        break;
                    default:
                        break;
                }
#ifdef __APPLE__
                pthread_attr_t qosAttribute;
                if (0 == pthread_attr_init(&qosAttribute)) {
                    pthread_attr_set_qos_class_np(&qosAttribute, GetNativeQOS(_priority), 0);
                }
#endif
                error = pthread_setschedparam(pthread_self(), policy, &param);
            }
        }
        else {
            error = errno;
        }
#endif
    }
    return std::error_code(error, std::system_category());
}

const char* ToString(ThreadPriority priority)
{
    switch (priority) {
        case ThreadPriority::Auto:
            return "auto";
        case ThreadPriority::Low:
            return "low";
        case ThreadPriority::Normal:
            return "normal";
        case ThreadPriority::High:
            return "high";
        case ThreadPriority::Highest:
            return "highest";
        case ThreadPriority::Realtime:
            return "realtime";
        default:
            break;
    }
    return "";
}

} // namespace LiveKitCpp
