#ifndef __HANDLE_HPP__
#define __HANDLE_HPP__

#include "threads/mutex.hpp"
#include "util/atomic.hpp"
#include "handle/schedulerver.hpp"
#include <map>
#include <vector>

#define INSTRUMENTATION(a)
#define DEBUG_DATAHANDLE_increasedVersion(this, version)
#define DEBUG_TASK_DEPENDENCY_addWaitingVersion(a)
#define DEBUG_TASK_DEPENDENCY_addWaitingLock(a)
#define IF_DEBUG(a)

// ============================================================================
// Option: Global ID for each handle
// ============================================================================
template<typename Options, bool HandleId>
class Handle_GlobalId {};

template<typename Options>
class Handle_GlobalId<Options, true> {
private:
	size_t id;
public:
	static size_t global_handle_id;
	Handle_GlobalId() {
		id = Atomic::atomic_increase_nv(&global_handle_id);
	}
	size_t getGlobalId() { return id; }
};

template<typename Options>
size_t Handle_GlobalId<Options, true>::global_handle_id;

// ============================================================================
// Option: Lockable
// ============================================================================

template<typename Options, bool Lockable> class Handle_Lockable;
template<typename Options>
class Handle_Lockable<Options, false> : public Options::HandleParentType {
public:
  typedef typename Options::IHandleType IHandle;
  void increaseCurrentVersion() {
    IHandle *this_(static_cast<IHandle*>(this));
		IHandle::increaseCurrentVersionImpl();
    this_->notifyVersionListeners();
  }
};

template<typename Options>
class Handle_Lockable<Options, true> : public Options::HandleParentType {
public:
  typedef typename Options::IHandleType IHandle;
  typedef typename Options::HandleType Handle;
  typedef typename Options::TaskType ITask;
private:
	// reading the listener list requires this mutex
  Mutex lockListenerMutex;
  // lock listeners (on current version)
  std::vector<LockListener<Options> *> lockListeners;
  // If object is locked (1) or not (0)
  volatile size_t lock;

  // Notify lock listeners when the lock is released
	void notifyLockListeners() {
    IHandle *this_(static_cast<IHandle*>(this));

    INSTRUMENTATION( Time::Instrumentation(totalNotifyLockTime); )
		MutexLockReleasable mutexLock(lockListenerMutex);

		if (lockListeners.empty())
			return;

		std::vector<LockListener<Options> *> list;
		std::swap(list, lockListeners);
		mutexLock.release();

		// TODO: hand over lock to first task in locality order that can run ?

    for (size_t i = 0; i < list.size(); ++i) {
			//      DBG("WAKE LOCK " << DEBUG_DATAHANDLE_getName(*this) << "\n");
			((ITask *) list[i])->DAG_addLockEdge();
      list[i]->notifyLock(this_);
    }
	}

public:

	Handle_Lockable() : lock(0) {}

  bool getLock() {
    Handle *this_(static_cast<Handle *>(this));
    MutexLock mutexLock(this_->mutex);
    if (lock == 0) {
  		//DBG("getLock[GOT!]: "<< DEBUG_DATAHANDLE_getName(*this) <<std::endl);
      lock = 1;
      return true;
    }
		return false;
	}

  bool getLockOrNotify(LockListener<Options> *task) {
    Handle *this_(static_cast<Handle *>(this));
    MutexLock mutexLock(this_->mutex);
    if (lock == 0) {
  		//DBG("getLock[GOT!]: "<< DEBUG_DATAHANDLE_getName(*this) <<std::endl);
      lock = 1;
      return true;
    }
		//DBG("getLock[NOTIFY]: "<< DEBUG_DATAHANDLE_getName(*this) <<std::endl);
    {
      DEBUG_TASK_DEPENDENCY_addWaitingLock((ITask *)task);
			//			DAG_addEdge((void*)((ITask*)task)->getGlobalId(), this, "[style=dotted]");
			MutexLock mutexLock2(lockListenerMutex);
			lockListeners.push_back(task);
		}
    return false;
  }

	void releaseLock() {
		Handle *this_(static_cast<Handle*>(this));
		{
			MutexLock mutexLock(this_->mutex);
			//DBG("releaseLock: "<< DEBUG_DATAHANDLE_getName(*this) <<std::endl);
			lock = 0;
	  }
		notifyLockListeners();
	}

  void increaseCurrentVersion() {
		Handle *this_(static_cast<Handle*>(this));
    bool notifyLock = false;
    { 
      MutexLock mutexLock(this_->mutex);
      ++this_->version;
      DEBUG_DATAHANDLE_increasedVersion(this, version);
      if (lock != 0) {
        lock = 0;
        notifyLock = true;
      }
    }
    this_->notifyVersionListeners();
    if (notifyLock)
      notifyLockListeners();
  }
};

// ============================================================================
// Handle
// ============================================================================
template<typename Options>
class Handle
  :	public Handle_Lockable<Options, Options::Lockable>,
    public Handle_GlobalId<Options, Options::HandleId>
{
	template<typename, bool> friend class Handle_Lockable;
public:
	typedef typename Options::TaskType ITask;
private:
  // ### TODO: cache-line usage?
	
  // decisions taken while holding this mutex
  Mutex mutex;
	// reading the listener list requires this mutex
  Mutex versionListenerMutex;
  // next required version for each access type
  SchedulerVersion<Options> requiredVersion;
  // version listeners, per version
  std::map<size_t, std::vector<VersionListener<Options> *> > versionListeners;
  // Current version
	volatile size_t version;

	// Not copyable -- there must be only one data handle per data.
  Handle(const Handle &);
	const Handle &operator=(const Handle &);

	inline void notifyVersionListeners() {
		INSTRUMENTATION( Time::Instrumentation(totalNotifyTime); )

    const size_t t = version;

		MutexLockReleasable mutexLock(versionListenerMutex);

		typedef typename std::map<size_t, std::vector<VersionListener<Options> *> >::iterator iter_t;

		iter_t l(versionListeners.find(t));
		if (l == versionListeners.end())
			return;

		if (l->second.empty()) {
			versionListeners.erase(l);
			return;
    }

		std::vector<VersionListener<Options> *> list;
		std::swap(list, l->second);
		versionListeners.erase(l);
		mutexLock.release();

		//		DBG("::: NOTIFY " << DEBUG_DATAHANDLE_getName(*this) << " v" << version << "\n");

		// HANDLE FIRST IN A SPECIAL WAY?
		// set prefered thread of first to current thread,
		// and handle others as usual?
		//		list[0]->notifyDataVersion(this);
    for (size_t i = 0; i < list.size(); ++i) {
			//			DBG("WAKE VERSION " << DEBUG_TASK_getName(*(ITask *) list[i]) << " VERSION " << DEBUG_DATAHANDLE_getName(*this) << "\n");
			((ITask *) list[i])->DAG_addVersionEdge();
      list[i]->notifyVersion(this);
    }
  }

	void increaseCurrentVersionImpl() {
		MutexLock mutexLock(mutex);
		++version;
		DEBUG_DATAHANDLE_increasedVersion(this, version);
	}

public:
  Handle() : version(0) {}
	~Handle() {}

  inline size_t getCurrentVersion() { return version; }

  // scheduler API {
  inline size_t schedulerGetRequiredVersion(int type) { 
		return requiredVersion.get(type); 
	}
	// ### TODO: schedulerIncrease not thread safe!
  inline void schedulerIncrease(int type) { requiredVersion.increase(type); }

  bool isVersionAvailableOrNotify(VersionListener<Options> *listener, size_t requiredVersion_) {
		MutexLock mutexLock(mutex);
    {
			if ( (int) (version - requiredVersion_) >= 0)
				return true;
		}
		//		DBG("::: REGISTER " << DEBUG_DATAHANDLE_getName(*this) << " v" << requiredVersion << "\n");
		{
      DEBUG_TASK_DEPENDENCY_addWaitingVersion((ITask *)listener);
			MutexLock listenerLock(versionListenerMutex);
			//			DAG_addEdge((void*)((ITask*)listener)->getGlobalId(), this, "[weight=8]");
			versionListeners[requiredVersion_].push_back(listener);
		}
    return false;
  }
};

#endif // __HANDLE_HPP__
