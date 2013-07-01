#ifndef __DEFAULTS_HPP__
#define __DEFAULTS_HPP__

#include "options/readwriteadd.hpp"
#include "options/defaultscheduler.hpp"
#include "options/defaulthwmodel.hpp"
#include "options/stealorder.hpp"

template <typename Options>
struct DefaultOptions {
    template<typename T2> struct Alloc {
        typedef std::allocator<T2> type;
    };
    typedef DefaultScheduler<Options> Scheduler;
    typedef DefaultStealOrder<Options> StealOrder;
    typedef ReadWriteAdd AccessInfoType;
    typedef unsigned int version_t;
    typedef Handle_<Options> HandleType;
    typedef TaskBase_<Options> TaskBaseType;
    template<int N> struct TaskType {
        typedef Task_<Options, N> type;
    };
    typedef TaskDynamic_<Options> TaskDynamicType;
    typedef DefaultHardwareModel HardwareModel;

    // Dependency Checking Options
    struct LazyDependencyChecking {};
    struct EagerDependencyChecking {};
    typedef LazyDependencyChecking DependencyChecking;

    // Instrumentation
    struct TaskExecutorNoInstrumentation {
        static void init() {}
        static void destroy() {}
        static void runTaskBefore() {}
        static void runTaskAfter() {}
        static void taskNotRunDeps() {}
        static void taskNotRunLock() {}
        static void setMainThreadFlag() {}
    };
    typedef TaskExecutorNoInstrumentation TaskExecutorInstrumentation;
};

#endif // __DEFAULTS_HPP__
