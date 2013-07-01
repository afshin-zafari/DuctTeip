#ifndef __WORKERTHREAD_HPP__
#define __WORKERTHREAD_HPP__

#include "threads/threads.hpp"
#include "threads/mutex.hpp"
#include "threads/barrier.hpp"
#include "util/circularbuffer.hpp"

#include <stdio.h>

#define DEBUG_TASKTIME_stoleWork()
#define DEBUG_TASKTIME_WAIT_FOR_WORK
#define DEBUG_TASKTIME_beforeWaitForWork()
#define DEBUG_THREADID2NUM_registerThread(id)
#define DEBUG_TASK_setThreadID(task, id);

// ===========================================================================
// Option ThreadWorkspace
// ===========================================================================
template<typename Options, bool threadWorkspace>
class WorkerThread_GetThreadWorkspace {
public:
	inline void resetWorkspaceIndex() {}
};

template<typename Options>
class WorkerThread_GetThreadWorkspace<Options, true> {
private:
	std::vector<char> workspace;
	size_t index;
public:
 	WorkerThread_GetThreadWorkspace() : workspace(131072), index(0) {}

	void resetWorkspaceIndex() { index = 0; }

	void *getThreadWorkspace(size_t size) {
		const size_t padded_size = size + HardwareModel::CACHE_LINE_SIZE;
		if (index+padded_size > workspace.size()) {
			fprintf(stderr, "### out of workspace allocated=%d/%d, requested=%d\n",
							index, workspace.size(), size);
			exit(-1);
		}
		void *res = &workspace[index];
		index += padded_size;
		return res;
	}
};

// ============================================================================
// Option Lockable
// ============================================================================
template<typename Options, bool> struct WorkerThread_Lockable;
template<typename Options> struct WorkerThread_Lockable<Options, true> {
	inline bool lockResources(typename Options::TaskType *task) {
 		return task->lockResourcesOrNotify();
 	}
};
template<typename Options> struct WorkerThread_Lockable<Options, false> {
	inline bool lockResources(typename Options::TaskType *task) {
 		return true;
 	}
};

// ============================================================================
// Option CurrentTask: Remember current task
// ============================================================================
template<typename Options, bool> class WorkerThread_CurrentTask;
template<typename Options> class WorkerThread_CurrentTask<Options, true> {
private:
	typename Options::TaskType *currentTask;
public:
	inline void rememberTask(typename Options::TaskType *task) {
		currentTask = task;
 	}
	WorkerThread_CurrentTask() : currentTask(0) {}
	inline typename Options::TaskType *getCurrentTask() { return currentTask; }
};
template<typename Options> class WorkerThread_CurrentTask<Options, false> {
public:
	inline void rememberTask(typename Options::TaskType *task) {}
};

// ============================================================================
// Option PassThreadId: Task called with thread id as argument
// ============================================================================
template<typename Options, bool PassThreadId> struct WorkerThread_PassThreadId;
template<typename Options> struct WorkerThread_PassThreadId<Options, true> {
	typedef typename Options::WorkerType WorkerThread;
	inline void runTask(typename Options::TaskType *task) {
		task->runTask(static_cast<WorkerThread*>(this)->id);
	}
};
template<typename Options> struct WorkerThread_PassThreadId<Options, false> {
	inline void runTask(typename Options::TaskType *task) { 
		task->runTask();	
	}
};

// ============================================================================
// Option Renamed && PassThreadId
// ============================================================================
template<typename Options, bool PassThreadId> struct WorkerThread_RenamedPassThreadId;
template<typename Options> struct WorkerThread_RenamedPassThreadId<Options, true> {
	inline void runTaskRenamed(typename Options::TaskType *task) { 
		task->runTaskRenamed(static_cast<typename Options::WorkerType*>(this)->id);
	}
};
template<typename Options> struct WorkerThread_RenamedPassThreadId<Options, false> {
	inline void runTaskRenamed(typename Options::TaskType *task) {
		task->runTaskRenamed();
	}
};

// ============================================================================
// Option Renaming
// ============================================================================
template<typename Options, bool> struct WorkerThread_Renaming;
template<typename Options> struct WorkerThread_Renaming<Options, true> 
	: public WorkerThread_RenamedPassThreadId<Options, Options::PassThreadId>
{
  typedef typename Options::WorkerType WorkerThread;

	inline bool runRenamed(typename Options::TaskType *task) {
		WorkerThread *this_(static_cast<WorkerThread *>(this));
 		if (!task->canRunRenamed())
 			return false;
		
 		// try to perform operatin in-place
 		DEBUG_TASK_setThreadID(*task, id);
 		if (!Options::Renaming_RunAllRenamed && task->lockResources()) {
 			this_->runTask(task);
 		}
 		else {
 			// run renamed
 			task->setRenamed(true);
 			this_->runTaskRenamed(task);
 		}
 		return true;
 	}
};
template<typename Options> struct WorkerThread_Renaming<Options, false> {
	inline bool runRenamed(typename Options::TaskType *task) { return false; }
};

// ============================================================================
// WorkerThread
// ============================================================================
template<typename Options>
class WorkerThread
	: public BarrierProtocolThread,
		public WorkerThread_PassThreadId<Options, Options::PassThreadId>,
		public WorkerThread_Renaming<Options, Options::Renaming>,
		public WorkerThread_Lockable<Options, Options::Lockable>,
		public WorkerThread_CurrentTask<Options, Options::CurrentTask>,
		public WorkerThread_GetThreadWorkspace<Options, Options::ThreadWorkspace>
{
	template<typename, bool> friend struct WorkerThread_PassThreadId;
  template<typename, bool> friend struct WorkerThread_RenamedPassThreadId;
	template<typename, bool> friend struct WorkerThread_Renaming;
public:
	typedef typename Options::TaskType ITask;
	typedef typename Options::ThreadManagerType ThreadManager;

protected:
	// ### Cache line usage?
	int id;
  ThreadManager &parent;
	Thread *thread;
  CircularBuffer<ITask *> readyList;
	volatile bool terminateFlag;

  WorkerThread(const WorkerThread &);
  const WorkerThread &operator=(const WorkerThread &);

  // Called from this thread only
	ITask *getTaskInternal() {
		// get ready task, if any

		// DBG("get first ready task, if any" << std::endl);
		ITask *task = 0;
		if (readyList.get(task))
			return task;

		// try to steal tasks
		if (parent.stealTask(id, task)) {
			DEBUG_TASKTIME_stoleWork();
			return task;
		}
		return 0;
	}

  // Called from this thread only
	ITask *getTask() {
		DEBUG_TASKTIME_WAIT_FOR_WORK;
		DEBUG_TASKTIME_beforeWaitForWork();

		// Try to get work without locking
		ITask *task = getTaskInternal();
		if (task != 0)
			return task;

		// Lock and check for work the expensive way
		SignalLock lock(BarrierProtocolThread::getBarrierLock());
		for (;;) {

			// Check if new work has arrived
			task = getTaskInternal();
			if (task != 0)
				return task;

			// Wait for work

			if (BarrierProtocolThread::waitAtBarrier(lock)) {
				// waiting at barrier
				continue;
			}
			else if (terminateFlag) {
				lock.release();
				//DBG("Terminate\n");
				ThreadUtil::exit();
				return 0;
			}
			else {
				// wait for work
				//DBG("wait for work\n");
				lock.wait();
			}
		}
	}

  // Called from this thread only
	void mainLoop() {

		for (;;) {
			ITask *task = getTask();
      if (task == 0)
        return;

			this->rememberTask(task);
			WorkerThread_GetThreadWorkspace<Options, Options::ThreadWorkspace>::resetWorkspaceIndex();

			if (this->runRenamed(task))
				continue;

			if (!this->lockResources(task)) {
				// ### should we wake everybody waiting in a barrier here?
				// DEBUG_TASK_DEPENDENCY_checkNotWaiting(task.get());
				continue;
			}
			DEBUG_TASK_setThreadID(*task, id);
			this->runTask(task);
		}
	}

public:
  // Called from this thread only
  WorkerThread(int id_, ThreadManager &parent_)
    : BarrierProtocolThread(parent_.barrierProtocol),
			id(id_), parent(parent_), terminateFlag(false) {
  }

  ~WorkerThread() {
	}

  // Called from other threads
  void setTerminateFlag() {
    terminateFlag = true;
    Atomic::memory_fence();
  }

	void run(Thread *thread_, Barrier &startBarrier) {
		this->thread = thread_;
    Atomic::memory_fence();
		srand(id);

		// wait for all threads to start
		startBarrier.wait();
		//		startBarrier.wait();

		DEBUG_THREADID2NUM_registerThread(id);

		mainLoop();
  }

  // Called from other threads
	inline void addReadyTaskFront(ITask *task) {
		readyList.push_front(task);
	}

  // Called from other threads
	inline void addReadyTaskBack(ITask *task) {
		readyList.push_back(task);
	}

  // Called from other threads
	inline bool steal(ITask *&task) {
		return readyList.steal(task);
	}

  // Called from other threads
	inline void join() {
		thread->join();
	}

	// For ThreadManager::getCurrentThread
	inline Thread *getThread() {
		return thread;
	}
};

#endif // __WORKERTHREAD_HPP__
