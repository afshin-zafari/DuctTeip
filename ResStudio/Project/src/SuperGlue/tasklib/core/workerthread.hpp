#ifndef __WORKERTHREAD_HPP__
#define __WORKERTHREAD_HPP__

#include "platform/threads.hpp"
#include "platform/mutex.hpp"
#include "core/barrierprotocol.hpp"

template<typename Options> class TaskBase;

// ============================================================================
// WorkerThread
// ============================================================================
template<typename Options>
class WorkerThread
  : public TaskExecutor<Options>
{
    template<typename, typename> friend struct WorkerThread_PassThreadId;
protected:
    Thread *thread;
    bool terminateFlag;
    BarrierProtocol<Options> &barrierProtocol;

    WorkerThread(const WorkerThread &);
    const WorkerThread &operator=(const WorkerThread &);

    // Called from this thread only
    void mainLoop() {
        barrierProtocol.lock();

        for (;;) {

            // Check if new work has arrived
            TaskBase<Options> *task;
            {
                typename Log<Options>::Instrumentation t("getTaskInternal");
                task = TaskExecutor<Options>::getTaskInternal();
            }
            barrierProtocol.leaveOldBarrier(*this);
            if (task != 0) {
                barrierProtocol.release();
                executeTask(task);
                barrierProtocol.lock();
                continue;
            }

            if (barrierProtocol.waitAtBarrier(*this)) {
                // waiting at barrier
                continue;
            }
            else if (terminateFlag) {
//                Log<Options>::add("terminate");
                // terminate
                barrierProtocol.release();
                ThreadUtil::exit();
                return;
            }
            else {
                // wait for work
                barrierProtocol.waitForWork(); // must include memory barrier to reread 'terminateFlag'
            }
        }
    }

public:
    int mystate; // TODO: Only for spinwait?
    volatile bool waitAtBarrier; // TODO: Only for Signals

    // Called from this thread only
    WorkerThread(int id_, ThreadManager<Options> &tm_)
      : TaskExecutor<Options>(id_, tm_), terminateFlag(false), barrierProtocol(tm_.barrierProtocol), mystate(0),
        waitAtBarrier(false) {}

    ~WorkerThread() {}

    // Called from other threads
    void join() { thread->join(); }

    // For ThreadManager::getCurrentThread and logging
    Thread *getThread() { return thread; }

    // Called from other threads
    void setTerminateFlag() {
        terminateFlag = true;
        // no memory fence here, must be followed by a global wake signal which will take care of the fence
    }

    void run(Thread *thread_) {
        this->thread = thread_;
        Atomic::memory_fence_producer();
        srand(TaskExecutor<Options>::getId());

        ThreadManager<Options> &tm2(TaskExecutor<Options>::getThreadManager());
        tm2.waitToStartThread();
        mainLoop();
    }
};

#endif // __WORKERTHREAD_HPP__
