#ifndef __TASK_HPP__
#define __TASK_HPP__

// todo: support fixed-access tasks?
//   vector of accesses could be fixed

#include "handle/access.hpp"
#include "handle/handlelistener.hpp"
#include "util/atomic.hpp"
#include "dag.h"
#include <vector>

#define AND_DEBUG_TASK_MIXIN(ITask)
#define DEBUG_TASK_DEPENDENCY_removeWaitingVersion(this)
#define DEBUG_TASK_DEPENDENCY_removeWaitingLock(this)
#define DEBUG_TASK_DEPENDENCY_NOTIFY(this, handle)
#define DEBUG_TASK_DATALOCK_NOTIFY(this, handle)
#define DEBUG_SCHEDULER_taskDataLocked(this, i)
#define DEBUG_TASKTIME_beforeRunTask(task);
#define DEBUG_TASKTIME_afterRunTask()
#define IF_DEBUG(a)

//class ReleaseHandle {
//private:
//	bool renamed;
//  std::vector<Access> access;
//
//public:
//  ReleaseHandle(bool renamed_, std::vector<Access> &access_)
//		: renamed(renamed_) {
//    std::swap(access, access_);
//  }
//  ~ReleaseHandle() {
//		//DBG("ReleaseHandle");
//		//		if (!renamed) {
//			for (size_t i = 0; i < access.size(); ++i)
//				access[access.size()-i-1].finished();
//			//		}
//			//		else {
//			//			for (size_t i = 0; i < access.size(); ++i)
//			//				access[access.size()-i-1].finishedRenamed();
//			//		}
//  }
//};

// ============================================================================
// Option: Global ID for each task
// ============================================================================
template<typename Options, bool GlobalTaskId>
class ITask_GlobalId {};

template<typename Options>
class ITask_GlobalId<Options, true> {
private:
	size_t id;
public:
	static size_t global_task_id;
	ITask_GlobalId() {
		id = Atomic::atomic_increase_nv(&global_task_id);
	}
	size_t getGlobalId() { return id; }
};

template<typename Options>
size_t ITask_GlobalId<Options, true>::global_task_id;

// ============================================================================
// Option Renamed
// ============================================================================
template<typename Options, bool PassThreadId>
class ITask_PassThreadIdRenamed;

template<typename Options>
class ITask_PassThreadIdRenamed<Options, true> {
protected:
	// user-supplied functionality
	virtual void runRenamed(int /*id*/) {};

public:
  typedef typename Options::TaskType ITask;

  // called from scheduler to run the task
	inline void runTaskRenamed(int id) {
		DEBUG_TASKTIME_beforeRunTask(this);
		runRenamed(id);
		DEBUG_TASKTIME_afterRunTask();
		static_cast<ITask*>(this)->finished();
	}
};

template<typename Options>
class ITask_PassThreadIdRenamed<Options, false> {
protected:
	// user-supplied functionality
	virtual void runRenamed() {};

public:
  typedef typename Options::TaskType ITask;

	// called from scheduler to run the task
	inline void runTaskRenamed() {
		DEBUG_TASKTIME_beforeRunTask(this);
		runRenamed();
		DEBUG_TASKTIME_afterRunTask();
		static_cast<ITask*>(this)->finished();
	}
};

// ============================================================================
// Option Renaming
// ============================================================================
template<typename Options, bool Renaming>
class ITask_Renaming;

template<typename Options>
class ITask_Renaming<Options, true>
	: public ITask_PassThreadIdRenamed<Options, Options::PassThreadId> {
public:
	typedef typename Options::TaskType ITask;
	typedef typename Options::IHandleType IHandle;
	typedef typename Options::AccessType Access;
	typedef typename Options::AccessInfoType AccessInfo;
	typedef typename AccessInfo::Type AccessType;
protected:
	bool renamed;

	// called when task is finished (from runTask)
	// increases data versions and DELETES THE TASK.
  void finished() {
		ITask *this_(static_cast<ITask *>(this));
		const size_t numAccess = this_->getNumAccess();
		if (isRenamed()) {
			for (size_t i = 0; i < numAccess; ++i)
				this_->getAccess(numAccess-i-1).finishedRenamed();
		}
		else {
			for (size_t i = 0; i < numAccess; ++i)
				this_->getAccess(numAccess-i-1).increaseCurrentVersion();
		}
		this_->deleteSelf();
	}

public:
	ITask_Renaming() : renamed(false) {}

  inline void registerRenamedAccess(AccessType type, IHandle *handle) {
		ITask *this_(static_cast<ITask*>(this));
#ifdef DEBUG_ACCESS
		for (int i = 0; i < access.size(); ++i) {
			assert(access[i].getHandle() != handle);
		}
#endif
    this_->access.push_back(Access(type, handle, 0));
  }

	// renaming
	//	virtual void runRenamed(ThreadManager<WorkerThread<ITask> > &tm) {}
	virtual bool canRunRenamed() { return false; }

	// task-associated data. move to own class
	void setRenamed(bool isRenamed) { renamed = isRenamed; }
	bool isRenamed() { return renamed; }
};

template<typename Options>
class ITask_Renaming<Options, false> {
public:
	typedef typename Options::TaskType ITask;
protected:
	// called when task is finished (from runTask)
	// increases data versions and DELETES THE TASK.
  inline void finished() {
		ITask *this_(static_cast<ITask*>(this));
		const size_t numAccess = this_->getNumAccess();
		for (size_t i = 0; i < numAccess; ++i)
			static_cast<ITask*>(this)->getAccess(numAccess-i-1).increaseCurrentVersion();
		this_->deleteSelf();
	}
};

// ============================================================================
// Option PassThreadId
// ============================================================================
template<typename Options, bool PassThreadId>
class ITask_PassThreadId;

template<typename Options>
class ITask_PassThreadId<Options, true> {
protected:
	// user-supplied functionality
	virtual void run(int id) = 0;

public:
  typedef typename Options::TaskType ITask;

	// called from scheduler to run the task
	inline void runTask(int id) {
		DEBUG_TASKTIME_beforeRunTask(this);
		run(id);
		DEBUG_TASKTIME_afterRunTask();
		static_cast<ITask*>(this)->finished();
	}
};

template<typename Options>
class ITask_PassThreadId<Options, false> {
protected:
	// user-supplied functionality
	virtual void run() = 0;

public:
  typedef typename Options::TaskType ITask;

	// called from scheduler to run the task
	inline void runTask() {
		DEBUG_TASKTIME_beforeRunTask(this);
		run();
		DEBUG_TASKTIME_afterRunTask();
		static_cast<ITask*>(this)->finished();
	}
};

// ============================================================================
// Option Lockable
// ============================================================================
template<typename Options, bool Lockable>
class ITask_Lockable {};

template<typename Options>
class ITask_Lockable<Options, true> 
	: public LockListener<Options>
{
public:
  typedef typename Options::TaskType ITask;
  typedef typename Options::IHandleType IHandle;

	// called by scheduler
	// must have at most one resourcelock listener
	// => when we get a callback we do not have to
	// worry about several simultaneous callbacks.

  bool lockResourcesOrNotify() {
		ITask *this_(static_cast<ITask*>(this));
		const size_t numAccess = this_->getNumAccess();
		for (size_t i = 0; i < numAccess; ++i) {
			if (!this_->getAccess(i).getLockOrNotify(this)) {
				DEBUG_SCHEDULER_taskDataLocked(*this, i);
				for (size_t j = 0; j < i; ++j)
					this_->getAccess(i-j-1).releaseLock();
				return false;
			}
		}
		return true;
	}

	// try-lock without callback. When several locks
	// are needed, this method is used.
  bool lockResources() {
		ITask *this_(static_cast<ITask*>(this));
		const size_t numAccess = this_->getNumAccess();
		for (size_t i = 0; i < numAccess; ++i) {
			if (!this_->getAccess(i).getLock()) {
				DEBUG_SCHEDULER_taskDataLocked(*this, i);
				for (size_t j = 0; j < i; ++j)
					this_->getAccess(i-j-1).releaseLock();
				return false;
			}
		}
		return true;
	}

	// callback: requested lock is available.
  void notifyLock(IHandle * IF_DEBUG(handle) ) {
		ITask *this_(static_cast<ITask*>(this));
		DEBUG_TASK_DEPENDENCY_removeWaitingLock(this);
		this_->tm->addReadyTaskToThread(this_, this_->preferredThread, false);
		DEBUG_TASK_DATALOCK_NOTIFY(*this, handle);
	}
};

// ============================================================================
// Option DAG
// ============================================================================
template<typename Options, bool DAG> struct ITask_DAG {
	inline void DAG_addLockEdge() {}
	inline void DAG_addVersionEdge() {}
};

template<typename Options> struct ITask_DAG<Options, true> {
	typedef typename Options::TaskType ITask;
	typedef typename Options::WorkerType WorkerThread;

	void DAG_addLockEdge() {
		ITask *this_(static_cast<ITask *>(this));

		WorkerThread *wt(this_->tm->getCurrentThread());
		const size_t selfId = this_->getGlobalId();
		const size_t currentId = wt->getCurrentTask()->getGlobalId();

		DAG_addEdge((void *) selfId, (void *) currentId, "[color=red]");
	}

	void DAG_addVersionEdge() {
		ITask *this_(static_cast<ITask *>(this));

		WorkerThread *wt(this_->tm->getCurrentThread());
		const size_t selfId = this_->getGlobalId();
		const size_t currentId = wt->getCurrentTask()->getGlobalId();

		DAG_addEdge((void *) selfId, (void *) currentId, "[color=blue]");
	}
};

// ============================================================================
// ITask
// ============================================================================
template<typename Options>
class ITask
	: public VersionListener<Options>,
		public ITask_PassThreadId<Options, Options::PassThreadId>,
    public ITask_Renaming<Options, Options::Renaming>,
		public ITask_Lockable<Options, Options::Lockable>,
		public ITask_GlobalId<Options, Options::TaskId>,
		public ITask_DAG<Options, Options::DAG>
		AND_DEBUG_TASK_MIXIN(ITask<Options>) {
	template<typename, bool> friend class ITask_PassThreadId;
	template<typename, bool> friend class ITask_PassThreadIdRenamed;
	template<typename, bool> friend class ITask_Renaming;
	template<typename, bool> friend class ITask_Lockable;
	template<typename, bool> friend struct ITask_DAG;
public:
  typedef typename Options::AccessType Access;
  typedef typename Options::ThreadManagerType ThreadManager;
	typedef typename Options::IHandleType IHandle;
	typedef typename Options::AccessInfoType AccessInfo;
	typedef typename AccessInfo::Type AccessType;

private:
  ThreadManager *tm;
	std::vector<Access> access;
	volatile size_t accessPtr;

  int preferredThread; // task-associated scheduler data: TODO move into own class
	bool stolen;         // task-associated data: TODO move into own class

protected:
  ITask() : accessPtr(0), preferredThread(0), stolen(false) {}
	virtual ~ITask() {};

  inline void deleteSelf() {
    delete this; // ### DANGEROUS?
	}

  inline void registerAccess(AccessType type, IHandle *handle) {
		//#ifdef DEBUG_ACCESS
		//		for (int i = 0; i < access.size(); ++i) {
		//			assert(access[i].getHandle() != handle);
		//		}
		//#endif
    access.push_back(Access(type, handle));
  }

	//  // called when splitting an existing data
	//	boost::shared_ptr<ReleaseHandle> getReleaseHandle() {
	//		return boost::shared_ptr<ReleaseHandle>
	//			(new ReleaseHandle(renamed, access));
	//	}

	// called from ThreadManager when a subtask is added
	inline void setPreferredThread(int threadId) {
		preferredThread = threadId;
	}

	//	// called from ThreadManager when task is added
	//	inline size_t getRandomSliceIndex() {
	//		const size_t numAccess = getNumAccess();
	//		if (numAccess > 0) {
	//      const size_t dataIndex = rand() % numAccess;
	//      return getAccess(dataIndex).getHandle()->getSliceIndex();
	//			//return getAccess(0).getHandle()->getSliceIndex();
	//		}
	//		return rand();
	//	}

public: // for debugging	
	size_t getNumAccess() const { return access.size(); }
	inline const Access &getAccess(size_t i) const { return access[i]; }

	// called by scheduler when added
	void schedule(ThreadManager *tm_) {
		this->tm = tm_;
		// increaseSchedulerVersions
		for (size_t i = 0; i < access.size(); ++i)
			// ### TODO: schedulerIncrease not thread safe!
			access[i].schedulerIncrease();
	}

	// called by scheduler.
	bool areDependenciesSolvedOrNotify() {
		//		DBG("checkdep" << std::endl);
		for (; accessPtr < access.size(); ++accessPtr) {
			if (!access[accessPtr].isVersionAvailableOrNotify(this)) {
				//DBG(DEBUG_TASK_getName(*this) << ": input " << DEBUG_DATAHANDLE_getName(*access[accessPtr].getHandle()) << " not ready"<< std::endl);
				return false;
			}
		}
		return true;
	}

	// callback: requested version is available.
  void notifyVersion(IHandle * /*handle*/) {
		if (!areDependenciesSolvedOrNotify())
			return;

		DEBUG_TASK_DEPENDENCY_removeWaitingVersion(this);
		tm->addReadyTaskToThread(this, preferredThread, false);
		DEBUG_TASK_DEPENDENCY_NOTIFY(*this, handle);
	}

	// task-associated data. move to own class
	void setStolen(bool isStolen) { stolen = isStolen; }
	bool isStolen() { return stolen; }
};

#endif // __TASK_HPP__
