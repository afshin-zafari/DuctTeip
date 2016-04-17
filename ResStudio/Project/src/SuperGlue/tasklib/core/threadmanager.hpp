#ifndef __THREADMANAGER_HPP__
#define __THREADMANAGER_HPP__

// ThreadManager
//
// Creates and owns the worker threads and task pools.

#include "platform/threads.hpp"
#include "core/barrierprotocol.hpp"

template <typename Options> class TaskBase;
template <typename Options> class TaskQueue;
template <typename Options> class WorkerThread;
template <typename Options> class ThreadManager;
template <typename Options> class Log;

namespace detail {

// ===========================================================================
// Option WorkingManager
// ===========================================================================
template<typename Options, typename T = void>
struct ThreadManager_WorkingManager {
     static size_t getNumQueues(const size_t numThreads) { return numThreads+1; }

     void registerTaskQueues() const {
        ThreadManager<Options> *this_((ThreadManager<Options> *)(this));
        this_->taskQueues[getNumQueues(this_->getNumThreads())-1] = &this_->barrierProtocol.getTaskQueue();
    }
};

// ===========================================================================
// Option ThreadWorkspace
// ===========================================================================
template<typename Options, typename T = void>
struct ThreadManager_GetThreadWorkspace {};

template<typename Options>
struct ThreadManager_GetThreadWorkspace<Options, typename Options::ThreadWorkspace> {
    void *getWorkspace(TaskExecutor<Options> *taskExecutor, size_t size) const {
        return taskExecutor->getThreadWorkspace(size);
    }
};

// ===========================================================================
// Option GetCurrentThread
// ===========================================================================
template<typename Options> // ### TODO: Not possible to disable?
struct ThreadManager_GetCurrentThread {
    WorkerThread<Options> *getCurrentThread() const {
        const ThreadManager<Options> *this_((const ThreadManager<Options> *)(this));
        const ThreadIDType t(ThreadUtil::getCurrentThreadId());
        for (size_t i = 0; i < this_->numThreads; ++i)
            if (this_->threads[i]->getThread()->getThreadId() == t)
                return this_->threads[i];
        std::cerr << "getCurrentThread() failed" << std::endl;
        exit(1);
        return 0;
    }
};

// ===========================================================================
// Option PauseExecution
// ===========================================================================
template<typename Options, typename T = void>
struct ThreadManager_PauseExecution {
    static bool mayExecute() { return true; }
};

template<typename Options>
struct ThreadManager_PauseExecution<Options, typename Options::PauseExecution> {
    bool flag;
    ThreadManager_PauseExecution() : flag(false) {}
    void setMayExecute(bool flag_) { flag = flag_; }
    bool mayExecute() const { return flag; }
};

} // namespace detail

// ===========================================================================
// ThreadManager
// ===========================================================================
template<typename Options>
class ThreadManager
  : public detail::ThreadManager_GetCurrentThread<Options>,
    public detail::ThreadManager_GetThreadWorkspace<Options>,
    public detail::ThreadManager_PauseExecution<Options>,
    public detail::ThreadManager_WorkingManager<Options>
{
    template<typename> friend struct ThreadManager_GetCurrentThread;
    template<typename, typename> friend struct ThreadManager_GetThreadWorkspace;
    template<typename, typename> friend struct ThreadManager_PauseExecution;
    template<typename, typename> friend struct ThreadManager_WorkingManager;

private:
    const size_t numThreads;

public:
    BarrierProtocol<Options> barrierProtocol;
    Barrier *startBarrier; // could be released after startup

private:
public:

    WorkerThread<Options> **threads;
    TaskQueue<Options> **taskQueues;

    ThreadManager(const ThreadManager &);
    ThreadManager &operator=(const ThreadManager &);

    // called from thread starter. Can be called by many threads concurrently
    void registerThread(int id, WorkerThread<Options> *wt) {
        threads[id] = wt;
        taskQueues[id] = &wt->getTaskQueue();
        Log<Options>::registerThread(id+1);
    }

    // ===========================================================================
    // WorkerThreadStarter: Helper thread to start Worker thread
    // ===========================================================================
    template<typename Ops>
    class WorkerThreadStarter : public Thread {
    private:
        int id;
        ThreadManager &tm;

    public:
        WorkerThreadStarter(int id_,
                            ThreadManager &tm_)
        : id(id_), tm(tm_) {}

        void run() {
            // allocate Worker on thread
            WorkerThread<Ops> *wt = new WorkerThread<Ops>(id, tm);
            tm.registerThread(id, wt);
            wt->run(this);
        }
    };

    static bool dependenciesReadyAtAdd(TaskBase<Options> *task, typename Options::EagerDependencyChecking) {
        return task->areDependenciesSolvedOrNotify();
    }

    static bool dependenciesReadyAtAdd(TaskBase<Options> *, typename Options::LazyDependencyChecking) {
        return true;
    }

    // adds a task. One is required to invoke signalNewWork() after tasks have been added,
    // but it is enough to do this once after adding several tasks.
    void addTaskNoSignal(TaskBase<Options> *task, int cpuid) {
        if (!dependenciesReadyAtAdd(task, typename Options::DependencyChecking()))
            return;

        const size_t queueIndex = Options::Scheduler::place(task, cpuid, getNumQueues());
        taskQueues[queueIndex]->push_back(task);
    }

    public:
    ThreadManager(int numThreads_ = -1)
      : numThreads(numThreads_ == -1 ? (ThreadUtil::getNumCPUs()-1) : numThreads_),
        barrierProtocol(*this)
    {
        ThreadUtil::setAffinity(Options::HardwareModel::cpumap(ThreadUtil::getNumCPUs()-1)); // force master thread to run on last cpu.
        taskQueues = new TaskQueue<Options> *[detail::ThreadManager_WorkingManager<Options>::getNumQueues(numThreads)];
        detail::ThreadManager_WorkingManager<Options>::registerTaskQueues();

        threads = new WorkerThread<Options>*[numThreads];
        startBarrier = new Barrier(numThreads+1);
        Log<Options>::init(numThreads+1);
        for (size_t i = 0; i < numThreads; ++i) {
            WorkerThreadStarter<Options> *wts =
                    new WorkerThreadStarter<Options>(i, *this);
            wts->start(Options::HardwareModel::cpumap(i));
        }

        // waiting on a pthread barrier here instead of using
        // atomic increases made the new threads start much faster
        startBarrier->wait();

        /// Cannot destroy startbarrier
        /// unless we are certain that we are the last one into
        /// the barrier, which we cannot know. it is not enough
        /// that everyone has reached the barrier to destroy it

    }

    ~ThreadManager() {
        assert(detail::ThreadManager_PauseExecution<Options>::mayExecute());
        barrier();

        for (size_t i = 0; i < numThreads; ++i)
            threads[i]->setTerminateFlag();

        barrierProtocol.signalNewWork();

        for (size_t i = 0; i < numThreads; ++i)
            threads[i]->join();

        delete startBarrier;
        for (size_t i = 0; i < numThreads; ++i)
            delete threads[i];

        delete[] threads;
    }

    size_t getNumThreads() const { return numThreads; }
    TaskQueue<Options> **getTaskQueues() const { return taskQueues; }
    size_t getNumQueues() const { return detail::ThreadManager_WorkingManager<Options>::getNumQueues(numThreads); }
    WorkerThread<Options> *getWorker(size_t i) { return threads[i]; }

    // INTERFACE TO HANDLE (ADDS TASKS OF THEIR OWN) {

    void signalNewWork() { barrierProtocol.signalNewWork(); }

    // }

    // called from worker threads during startup
    void waitToStartThread() {
        startBarrier->wait();
        while (!detail::ThreadManager_PauseExecution<Options>::mayExecute())
            Atomic::memory_fence_consumer();
    }

    // USER INTERFACE {

    class TaskAdder {
    private:
        ThreadManager &tm;
        TaskAdder(ThreadManager &tm_) : tm(tm_) {}
    public:
        void addTask(TaskBase<Options> *task, int cpuid = 0) {
            tm.addTaskNoSignal(task, cpuid);
        }
        ~TaskAdder() {
            tm.signalNewWork();
        }
    };

    TaskAdder getTaskAdder() { return TaskAdder(*this); }

    void addTask(TaskBase<Options> *task, int cpuid = 0) {
        addTaskNoSignal(task, cpuid);
        barrierProtocol.signalNewWork();
    }

    void barrier() {
        barrierProtocol.barrier();
    }

    void barrier(int *value) {
        barrierProtocol.barrier(value);
    }

    static size_t suggestNumThreads() { return ThreadUtil::getNumCPUs()-1; }
    // }
};

#endif // __THREADMANAGER_HPP__
