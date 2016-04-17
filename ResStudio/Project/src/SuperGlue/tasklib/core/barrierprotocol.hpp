#ifndef __BARRIER_HPP__
#define __BARRIER_HPP__

#include "platform/atomic.hpp"
#include "platform/mutex.hpp"
#include "core/log.hpp"

template <typename Options> class ThreadManager;
template <typename Options> class WorkerThread;

template <typename Options, typename T = void>
class BarrierProtocol
  : public TaskExecutor<Options>
{
private:
    size_t barrierCounter;
    size_t state;
    size_t abort;

    bool participate() {
        TaskBase<Options> *task;
        {
            typename Log<Options>::Instrumentation("getTaskInternal");
            task = TaskExecutor<Options>::getTaskInternal();
        }
        if (task == 0)
            return false;
        TaskExecutor<Options>::executeTask(task);
        return true;
    }

public:
    BarrierProtocol(ThreadManager<Options> &tm_)
      : TaskExecutor<Options>(tm_.getNumQueues()-1, tm_),
        barrierCounter(0), state(0)
    {
        Options::TaskExecutorInstrumentation::setMainThreadFlag();
    }

    void waitForWork() {
        Atomic::rep_nop();
    }

    void waitAfterBarrier(WorkerThread<Options> &wwt) {
        Log<Options>::push(LogTag::Barrier());
        // if we have new work in our queue, then abort
        if (wwt.getTaskQueue().gotWork()) {
            abort = 1;
            Atomic::memory_fence_producer(); // propagate abort before updating state
            const size_t count = Atomic::decrease_nv(&barrierCounter);
            if (count == 0) {
                state = 0;
                Atomic::memory_fence_producer();
            }
            Log<Options>::pop(LogTag::Barrier(), "waitAfterBarrier-newwork1");
            return;
        }

        // leave barrier
        const size_t seenstate = state;
        const size_t count = Atomic::decrease_nv(&barrierCounter);
        if (count == 0) {
            // last to wait for barrier checks barrier task queue. a bit too late?
            if (TaskExecutor<Options>::getTaskQueue().gotWork()) {
                abort = 1;
                Atomic::memory_fence_producer(); // propagate abort before updating state
                Log<Options>::pop(LogTag::Barrier(), "waitAfterBarrier-newwork2");
            }
            else
                Log<Options>::pop(LogTag::Barrier(), "waitAfterBarrier-Last");

            state = 0;
            Atomic::memory_fence_producer();
            return;
        }

        // wait for everyone to leave barrier
        for (;;) {
            if (seenstate != state) {
                Log<Options>::pop(LogTag::Barrier(), "waitAfterBarrier-newstate");
                return;
            }
            if (abort == 1) {
                Log<Options>::pop(LogTag::Barrier(), "waitAfterBarrier-abort");
                return;
            }
            Atomic::rep_nop(); // this includes a memory barrier to force 'state' and 'abort' to be reread
        }
    }

    // Leave old barrier
    bool leaveOldBarrier(WorkerThread<Options> &wwt) {
        if (wwt.mystate != 1)
            return false;

        // currently aborting state 1

        if (state != 2) // continue waiting?
            return true;

        wwt.mystate = 0;
        Log<Options>::add(LogTag::Barrier(), "leaveOldBarrier-done");
        waitAfterBarrier(wwt);
        return true;
    }

    // Called from WorkerThread: Wait at barrier if requested.
    bool waitAtBarrier(WorkerThread<Options> &wwt) {
        if (leaveOldBarrier(wwt))
            return true;

        // here we can be in state 2 if we have aborted state 2 early
        if (state != 1)
            return false;

        Log<Options>::push(LogTag::Barrier());
        const size_t numThreads(TaskExecutor<Options>::getThreadManager().getNumThreads());

        const size_t count = Atomic::increase_nv(&barrierCounter);

        Log<Options>::add(LogTag::Barrier(), "barrier", count);

        if (count == numThreads) {
            // last to enter the barrier, go to next state
            state = 2;
            Atomic::memory_fence_producer();
            waitAfterBarrier(wwt);
            Log<Options>::pop(LogTag::Barrier(), "waitAtBarrier-last");
            return true;
        }

        // wait at barrier
        for (;;) {
            if (state == 2) {
                Log<Options>::pop(LogTag::Barrier(), "waitAtBarrier-state2");
                waitAfterBarrier(wwt);
                return true;
            }
            if (wwt.getTaskQueue().gotWork()) {
                abort = 1;
                Atomic::memory_fence_producer();
                wwt.mystate = 1;
                Log<Options>::pop(LogTag::Barrier(), "waitAtBarrier-gotwork");
                return true;
            }
            if (abort == 1) {
                // go back to work, but remember that we need to leave the barrier
                // when it reaches state 2
                wwt.mystate = 1;
                Log<Options>::pop(LogTag::Barrier(), "waitAtBarrier-abort");
                return true;
            }
            Atomic::yield(); // this includes a memory barrier to force 'state' and 'abort' to be reread
        }
        return true;
    }

    void barrier(int *value) {
        Log<Options>::add(LogTag::Barrier(), "barrier-waitforvalue");
        while (*value == 0) {
            while (participate());
            Atomic::memory_fence_consumer(); // to make sure we dont read abort & state in wrong order
        }
        barrier();
    }

    // cannot be invoked by more than one thread at a time
    void barrier() {
        Log<Options>::push(LogTag::Barrier());
        while (participate());

        if (TaskExecutor<Options>::getThreadManager().getNumThreads() == 0) {
            Log<Options>::pop(LogTag::Barrier(), "barrier-alone");
            return;
        }

        do {
            Log<Options>::add(LogTag::Barrier(), "barrier-in");
            abort = 0;
            Atomic::memory_fence_producer();
            state = 1;
            Atomic::memory_fence_producer();
            while (state != 0) {
                while (participate());
                Atomic::rep_nop(); // this includes a memory barrier to force 'state' to be reread
            }
            Atomic::memory_fence_consumer(); // to make sure we dont read abort & state in wrong order
        } while (abort == 1);
        Log<Options>::pop(LogTag::Barrier(), "barrier-done");
    }

    void signalNewWork() {
        abort = 1;
        Log<Options>::add(LogTag::Barrier(), "signalNewWork-abort");
    }
    static void lock() {}
    static void release() {}
};


template <typename Options>
class BarrierProtocol<Options, typename Options::Signals>
  : public TaskExecutor<Options>
{
private:
    volatile size_t barrierCounter;
    volatile bool barrierAborted;
    volatile int state;
    Signal counterSignal; // only visible through counterLock
    SignalLock counterLock;
    Mutex barrierMutex;
    Signal stateSignal;

    // expected to hold counterLock (not needed)
    void notifyDone() {
        SignalLock stateLock(stateSignal);
        state = 1-state;
        counterLock.broadcast();
        stateLock.signal();
    }

    struct ScopedCounterLock {
        SignalLock &counterLock;
        bool released;
        explicit ScopedCounterLock(SignalLock &counterLock_)
        : counterLock(counterLock_), released(false) {
            counterLock.lock();
        }
        ~ScopedCounterLock() {
            if (!released)
                counterLock.release();
        }
        void broadcast() { counterLock.broadcast(); }
        void release() {
            counterLock.release();
            released = true;
        }
    };

public:
    BarrierProtocol(ThreadManager<Options> &tm_)
      : TaskExecutor<Options>(tm_.getNumQueues()-1, tm_),
        barrierCounter(0), barrierAborted(false),
        state(0), counterLock(counterSignal, true)
    {}

    // abort barrier (called from ThreadManager)
    void signalNewWork() {
        Log<Options>::add(LogTag::Barrier(), "signalNewWork-abort");
        ScopedCounterLock lock(counterLock);
        barrierAborted = true;
        lock.broadcast();
    }

    // called from ThreadManager
    void barrier() {
        ThreadManager<Options> &tm(TaskExecutor<Options>::getThreadManager());
        const size_t numThreads = tm.getNumThreads();

        // to prevent several threads from calling barrier() concurrently
        MutexLock barrierLock(barrierMutex);

        do {
            ScopedCounterLock lock(counterLock);
            barrierCounter = 0;
            barrierAborted = false;

            SignalLock stateLock(stateSignal);

            int mystate = state;

            for (size_t i = 0; i < numThreads; ++i) {
                assert(!tm.getWorker(i)->waitAtBarrier);
                tm.getWorker(i)->waitAtBarrier = true;
            }

            lock.broadcast();
            lock.release();
            while (state == mystate)
                stateLock.wait();

        } while (barrierAborted);
    }

    // Wait at barrier if requested. (Called from WorkerThread)
    bool waitAtBarrier(WorkerThread<Options> &wwt) {
        if (!wwt.waitAtBarrier)
            return false;

        wwt.waitAtBarrier = false;

        ++barrierCounter;
        int mystate = state;

        if (barrierCounter == TaskExecutor<Options>::getThreadManager().getNumThreads()) {
            notifyDone();
            return true;
        }

        while (state == mystate && !barrierAborted)
            counterLock.wait();
        return true;
    }

    // called from WorkerThread
    void waitForWork() {
        counterLock.wait();
    }

    // called from WorkerThread
    static bool leaveOldBarrier(WorkerThread<Options> &) { return false; } // not used
    void lock() { counterLock.lock(); }
    void release() { counterLock.release(); }
};

#endif // __BARRIER_HPP__
