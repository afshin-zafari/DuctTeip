#ifndef __THREADMANAGER_HPP__
#define __THREADMANAGER_HPP__

// ThreadManager
//
// Creates and owns the worker threads and task pools.

#include "threads/hardwaremodel.hpp"
#include "threads/mutex.hpp"
#include "threads/threads.hpp"
#include "threads/barrier.hpp"

#define AND_DEBUG_THREADMANAGER_MIXIN(a)
#define DEBUG_THREADID2NUM_init(a)
#define DEBUG_TASKTIME_addTask(a)
#define DEBUG_THREADID2NUM_getThreadID()
#define DEBUG_TASKTIME_afterAddTask()
#define DEBUG_TASKTIME_init(a)

// ===========================================================================
// Option ThreadWorkspace
// ===========================================================================
template<typename Options, bool threadWorkspace>
struct ThreadManager_GetThreadWorkspace {};

template<typename Options>
struct ThreadManager_GetThreadWorkspace<Options, true> {
  typedef typename Options::ThreadManagerType ThreadManager;
	void *getWorkspace(int i, size_t size) {
		ThreadManager *this_((ThreadManager *)(this));
		return this_->threads[i]->getThreadWorkspace(size);
	}
};

// ===========================================================================
// GetCurrentThread
// ===========================================================================
template<typename Options>
struct ThreadManager_GetCurrentThread {
  typedef typename Options::WorkerType WorkerThread;
  typedef typename Options::ThreadManagerType ThreadManager;
  WorkerThread *getCurrentThread() const {
		const ThreadManager *this_((const ThreadManager *)(this));
		const ThreadIDType t(ThreadUtil::getCurrentThreadId());
		for (size_t i = 0; i < this_->numThreads; ++i) {
			if (this_->threads[i]->getThread()->getThreadId() == t)
				return this_->threads[i];
		}
		return 0;
	}
};

// ===========================================================================
// ThreadManager
// ===========================================================================
template<typename Options>
class ThreadManager
  : public ThreadManager_GetCurrentThread<Options>,
		public ThreadManager_GetThreadWorkspace<Options, Options::ThreadWorkspace >
	AND_DEBUG_THREADMANAGER_MIXIN(ThreadManager) {
	template<typename> friend struct ThreadManager_GetCurrentThread;
	template<typename, bool> friend struct ThreadManager_GetThreadWorkspace;
public:
	const size_t numThreads;
	BarrierProtocol barrierProtocol;

public:
	typedef typename Options::TaskType TaskType;
  typedef typename Options::WorkerType WorkerThread;

private:

  // ### cache line usage?
  WorkerThread **threads; // ### volatile?
  char padd[HardwareModel::CACHE_LINE_SIZE - sizeof(WorkerThread **)];
  HardwareModel hwModel;
	Barrier *startBarrier; // ### could be released after startup!

  ThreadManager(const ThreadManager &);
  ThreadManager &operator=(const ThreadManager &);

	// ===========================================================================
	// WorkerThreadStarter: Helper thread to start Worker thread
	// ===========================================================================
	class WorkerThreadStarter : public Thread {
	private:
		int id;
		WorkerThread **ptr;
		ThreadManager &parent;
		Barrier &barrier;

	public:

		WorkerThreadStarter(int id_, 
												WorkerThread** ptr_, 
												ThreadManager &parent_,
												Barrier &barrier_)
			: id(id_), ptr(ptr_), parent(parent_), barrier(barrier_) {}

		void run() {
			WorkerThread *wt = new WorkerThread(id, parent);
      *(ptr) = wt;
			wt->run(this, barrier);
		}
	};

protected:
public:
  // INTERFACE TASK {

		// called from task when is have been woken as new dependencies are fulfilled
		// or contribution lock is released to wake a waiting task
    // ### SLOW! Need vectorized implementation! Cannot do signalNewWork after each task!!! ###
	void addReadyTaskToThread(TaskType *task, size_t threadIndex, bool addToFront) {
			assert(threadIndex >= 0);
			assert(threadIndex < numThreads);
			WorkerThread &t(*threads[threadIndex]);
			assert(task != NULL);
			{
				if (addToFront)
					t.addReadyTaskFront(task);
				else
					t.addReadyTaskBack(task);
        Atomic::memory_fence();
				barrierProtocol.signalNewWork();
			}
		}

  // }

public:
	ThreadManager(int numThreads_) 
	: numThreads(numThreads_),
		barrierProtocol(numThreads_)
	{
		ThreadUtil::setAffinity(ThreadUtil::getNumCPUs()-1); // force master thread to run on last cpu.

		threads = new WorkerThread*[numThreads];

		//DEBUG_THREADID2NUM_init(numThreads);
		
		startBarrier = new Barrier(numThreads+1);

		for (size_t i = 0; i < numThreads; ++i) {
			//			StartupData *sd = new StartupData();
			WorkerThreadStarter *wts =
				new WorkerThreadStarter(i, &threads[i], *this, *startBarrier);
			wts->start(i);
		}

		startBarrier->wait();

		/// Cannot destroy startbarrier
		/// unless we are certain that we are the last one into 
		/// the barrier, which we cannot know. it is not enough
		/// that everyone has reached the barrier to destroy it

		//DEBUG_TASKTIME_init(numThreads);
	}

	//	void start() {
	//		startBarrier->wait();
	//	}
	
	~ThreadManager() {
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

	// should only be called from WorkerThread
	bool stealTask(size_t id, TaskType *&dest) {
		for (size_t i = 0; i < numThreads-1; ++i) {
			if (i == id)
				continue;
			
			//DBG("steal (task " << id << ") from " << i << endl);
			if (threads[i]->steal(dest)) {
				//DBG("steal (task " << id << ") from " << i << " STEALING" << endl);
				dest->setStolen(true);
				return true;
			}
			//DBG("steal (task " << id << ") from " << i << " NO TASKS" << endl);
		}
		return false;
	}


  // USER INTERFACE {

    // called to create a continuation task from a task.
    // This means that the scheduler version isn't increased
    // as it was increased when the parent task was added.
    // 
    void addContinuation(TaskType *task) {
			DEBUG_TASKTIME_addTask(*task);
			task->setPreferredThread(DEBUG_THREADID2NUM_getThreadID());

			//			SignalLock workCounterLock(workCounterSignal);
			//			++workcounter;

			if (task->areDependenciesSolvedOrNotify())
				addReadyTaskToThread(task, task->preferredThread, true);

			DEBUG_TASKTIME_afterAddTask();
		}

    // called to create task from a task.
		// task gets assigned to calling worker
		// task added to front of queue
    void addSubTask(TaskType *task) {
			DEBUG_TASKTIME_addTask(*task);
			task->setPreferredThread(DEBUG_THREADID2NUM_getThreadID());

      task->increaseSchedulerVersions();

			if (task->areDependenciesSolvedOrNotify())
				addReadyTaskToThread(task, task->preferredThread, true);

			DEBUG_TASKTIME_afterAddTask();
		}

	  void addTask(TaskType *task, int cpuid = 0) {
			DEBUG_TASKTIME_addTask(*task);
			// ### TODO: ASSIGNMENT!
			//			size_t threadId = task->getRandomSliceIndex() % numThreads;
			// task->setPreferredThread(threadId);

			task->schedule(this);

			// ### TODO: ASSIGNMENT!
			if (task->areDependenciesSolvedOrNotify()) {
				addReadyTaskToThread(task, cpuid % numThreads, false);
			//				addReadyTaskToThread(task, task->preferredThread, false);
			}

			DEBUG_TASKTIME_afterAddTask();
		}

		inline void barrier() {
			barrierProtocol.barrier(threads);
		}
	// }
};

#endif // __THREADMANAGER_HPP__
