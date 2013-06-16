#ifndef __DEFAULTS_HPP__
#define __DEFAULTS_HPP__

#include "core/accesstypes.hpp"
#include "core/sched_default.hpp"
#include "core/hwmodel_default.hpp"
#include "core/stealorder.hpp"

template <typename Options>
struct DefaultOptions {

    struct Enable {};
    struct Disable {};

    template<typename T2> struct Alloc {
        typedef std::allocator<T2> type;
    };
    typedef DefaultScheduler<Options> Scheduler;
    typedef DefaultStealOrder<Options> StealOrder;
    typedef ReadWriteAdd AccessInfoType;
    typedef unsigned int version_t;
    typedef DefaultHardwareModel HardwareModel;

    // Types that can be overloaded
    typedef HandleDefault<Options> HandleType;
    typedef TaskBaseDefault<Options> TaskBaseType;
    template<int N> struct TaskType {
        typedef TaskDefault<Options, N> type;
    };

    // Features
    typedef Disable TaskName;            // tasks must implement a getName() method
    typedef Disable TaskId;              // tasks have a getGlobalId() method
    typedef Disable TaskPriorities;      // tasks must implement a getPriority() method
    typedef Disable TaskStealableFlag;   // tasks must implement a isStealable() method
    typedef Disable TaskStolenFlag;      // tasks have a isStolen() method
    typedef Disable HandleName;          // handles have setName() and getName() methods
    typedef Disable HandleId;            // handels have a getGlobalId() method
    typedef Enable Lockable;             // handles can be locked
    typedef Disable ThreadSafeSubmit;    // submitting tasks is thread safe
    typedef Disable SubTasks;            // TaskExecutor has addSubTask() method
    typedef Disable PassTaskExecutor;    // the Task::run() method is called with the current TaskExecutor as parameter
    typedef Disable ThreadWorkspace;     // TaskExecutor have a getThreadWorkspace() method
    typedef Disable ExecutorCurrentTask; // TaskExecutor have a getCurrentTask() method
    typedef Disable PauseExecution;      // No tasks are executed until setMayExecute(true) is called
    typedef Enable Stealing;             // Task stealing enabled
    typedef Enable ListQueue;

    // Size of ThreadWorkspace (only used if ThreadWorkspace is enabled)
    enum { ThreadWorkspace_size = 102400 };

    // Dependency Checking Options
    struct LazyDependencyChecking {};
    struct EagerDependencyChecking {};
    typedef LazyDependencyChecking DependencyChecking;

    // Contributions Options
    typedef Disable Contributions;

    // Logging Options
    typedef Disable Logging;
    typedef Disable Logging_DAG;

    // Instrumentation
    struct TaskExecutorNoInstrumentation {
        static void init() {}
        static void destroy() {}
        static void runTaskBefore(TaskBase<Options> *) {}
        static void runTaskAfter(TaskBase<Options> *) {}
        static void taskNotRunDeps() {}
        static void taskNotRunLock() {}
        static void setMainThreadFlag() {}
    };
    typedef TaskExecutorNoInstrumentation TaskExecutorInstrumentation;
    
    // Releasing task memory
    struct FreeTaskDefault {
        static void free(TaskBase<Options> *task) {
            delete task;
        }
    };
    typedef FreeTaskDefault FreeTask;
};

#endif // __DEFAULTS_HPP__
