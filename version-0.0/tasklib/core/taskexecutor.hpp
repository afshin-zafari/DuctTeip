#ifndef __TASKEXECUTOR_HPP__
#define __TASKEXECUTOR_HPP__

template<typename Options> class TaskQueue;
template<typename Options> class ThreadManager;
template<typename Options> class TaskExecutor;

namespace detail {

// ===========================================================================
// Option SubTasks
// ===========================================================================
template<typename Options, typename T = void>
struct TaskExecutor_SubTasks {};

template<typename Options>
struct TaskExecutor_SubTasks<Options, typename Options::SubTasks> {

    static bool checkDependencies(TaskBase<Options> *task, typename Options::LazyDependencyChecking) {
        return true;
    }
    static bool checkDependencies(TaskBase<Options> *task, typename Options::EagerDependencyChecking) {
        return task->areDependenciesSolvedOrNotify();
    }

    // called to create task from a task.
    // task gets assigned to calling task executor
    // task added to front of queue
    void addSubTask(TaskBase<Options> *task) {
        TaskExecutor<Options> *this_((TaskExecutor<Options> *)(this));

        if (!checkDependencies(task, typename Options::DependencyChecking()))
            return;

        this_->getTaskQueue().push_front(task);
    }
};

// ============================================================================
// Option NoStealing
// ============================================================================
template<typename Options, typename T = void>
struct TaskExecutor_NoStealing {
    template<typename Ops, typename T2 = void> struct SetStolen {
        static void setStolen(TaskBase<Options> *task) {};
    };
    template<typename Ops>
    struct SetStolen<Ops, typename Ops::StolenFlag> {
        static void setStolen(TaskBase<Options> *task) {
            task->setStolen(true);
        };
    };

    // Called from this thread only
    TaskBase<Options> *getTaskInternal() {
        TaskExecutor<Options> *this_(static_cast<TaskExecutor<Options> *>(this));

//        this_->readyList.dump();
        TaskBase<Options> *task = 0;
        if (this_->readyList.get(task))
            return task;

        // try to steal tasks
        if (Options::StealOrder::steal(this_->getThreadManager(), this_->getId(), task)) {
            Log<Options>::add(LogTag::Stealing(), "stolen", task);
            SetStolen<Options>::setStolen(task);
            return task;
        }

        return 0;
    }
};

template<typename Options>
struct TaskExecutor_NoStealing<Options, typename Options::NoStealing> {
    TaskBase<Options> *getTaskInternal() {
        TaskExecutor<Options> *this_(static_cast<TaskExecutor<Options> *>(this));

        TaskBase<Options> *task = 0;
        if (this_->readyList.get(task))
            return task;

        return 0;
    }
};

// ============================================================================
// Option TaskExecutor_PassTaskExecutor: Task called with task executor as argument
// ============================================================================
template<typename Options, typename T = void>
struct TaskExecutor_PassTaskExecutor {
    static void invokeTaskImpl(TaskBase<Options> *task) { task->run(); }
};
template<typename Options>
struct TaskExecutor_PassTaskExecutor<Options, typename Options::PassTaskExecutor> {
    void invokeTaskImpl(TaskBase<Options> *task) {
        task->run(static_cast<TaskExecutor<Options> *>(this));
    }
};

// ===========================================================================
// Option ThreadWorkspace
// ===========================================================================
template<typename Options, typename T = void>
class TaskExecutor_GetThreadWorkspace {
public:
    void resetWorkspaceIndex() {}
};

template<typename Options>
class TaskExecutor_GetThreadWorkspace<Options, typename Options::ThreadWorkspace> {
private:
    typename Options::template Alloc<char>::type allocator;
    char *workspace;
    size_t index;

public:
    TaskExecutor_GetThreadWorkspace() : index(0) {
        workspace = allocator.allocate(Options::ThreadWorkspace_size);
    }
    ~TaskExecutor_GetThreadWorkspace() {
        allocator.deallocate(workspace, Options::ThreadWorkspace_size);
    }
    void resetWorkspaceIndex() { index = 0; }

    void *getThreadWorkspace(const size_t size) {
        if (index > Options::ThreadWorkspace_size) {
            std::cerr
                << "### out of workspace. allocated=" << index << "/" << Options::ThreadWorkspace_size
                << ", requested=" << size << std::endl;
            exit(-1);
        }
        void *res = &workspace[index];
        index += size;
        return res;
    }
};

// ============================================================================
// Option CurrentTask: Remember current task
// ============================================================================
template<typename Options, typename T = void>
class TaskExecutor_CurrentTask {
public:
    void rememberTask(TaskBase<Options> *task) {}
};

template<typename Options>
class TaskExecutor_CurrentTask<Options, typename Options::CurrentTask> {
private:
    TaskBase<Options> *currentTask;
public:
    TaskExecutor_CurrentTask() : currentTask(0) {}

    void rememberTask(TaskBase<Options> *task) { currentTask = task; }
    TaskBase<Options> *getCurrentTask() { return currentTask; }
};

} // namespace detail

// ============================================================================
// TaskExecutor
// ============================================================================
template<typename Options>
class TaskExecutor
  : public detail::TaskExecutor_CurrentTask<Options>,
    public detail::TaskExecutor_GetThreadWorkspace<Options>,
    public detail::TaskExecutor_PassTaskExecutor<Options>,
    public detail::TaskExecutor_NoStealing<Options>,
    public detail::TaskExecutor_SubTasks<Options>,
    public Options::TaskExecutorInstrumentation
{
    template<typename, typename> friend struct TaskExecutor_NoStealing;
    template<typename, typename> friend struct TaskExecutor_PassThreadId;

private:
public:

    int id;
    ThreadManager<Options> &tm;
    TaskQueue<Options> readyList;

private:
public:

    void invokeTask(TaskBase<Options> *task) {

        Options::TaskExecutorInstrumentation::runTaskBefore();

        invokeTaskImpl(task);

        Options::TaskExecutorInstrumentation::runTaskAfter();

        task->finished(*this); // 600 cycles!?
    }

public:

    TaskExecutor(int id_, ThreadManager<Options> &tm_)
      : id(id_), tm(tm_)
    {
        Options::TaskExecutorInstrumentation::init();
    }

    ~TaskExecutor() {
        Options::TaskExecutorInstrumentation::destroy();
    }

    static bool dependenciesReadyAtExecution(TaskBase<Options> *task, typename Options::LazyDependencyChecking) {
        return task->areDependenciesSolvedOrNotify();
    }
    static bool dependenciesReadyAtExecution(TaskBase<Options> *task, typename Options::EagerDependencyChecking) {
        return true;
    }

    // Called from this thread only
    void executeTask(TaskBase<Options> *task) {
        if (!dependenciesReadyAtExecution(task, typename Options::DependencyChecking())) {
            Options::TaskExecutorInstrumentation::taskNotRunDeps();
            return;
        }

        // book-keeping
        detail::TaskExecutor_CurrentTask<Options>::rememberTask(task);
        detail::TaskExecutor_GetThreadWorkspace<Options>::resetWorkspaceIndex();
        task->applyOldContributionsBeforeRead();

        // run with contributions, if that is activated
        if (task->runContrib()) {
            invokeTask(task);
            return;
        }

        // run, lock if needed
        if (task->tryLock(*this))
            invokeTask(task);
        else
            Options::TaskExecutorInstrumentation::taskNotRunLock();
    }

    TaskQueue<Options> &getTaskQueue() { return readyList; }
    int getId() const { return id; }
    ThreadManager<Options> &getThreadManager() { return tm; }
};

#endif // __TASKEXECUTOR_HPP__

