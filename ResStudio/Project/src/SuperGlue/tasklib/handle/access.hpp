#ifndef __ACCESS_HPP__
#define __ACCESS_HPP__

#include "handlelistener.hpp"

#include <algorithm>
#include <cstring>
#include <cassert>

// ============================================================================
// Option Renaming
// ============================================================================
template<typename Options, bool Renaming>
class Access_Renaming {};

template<typename Options>
class Access_Renaming<Options, true> {
public:
  typedef typename Options::AccessType Access;
  typedef typename Options::AccessInfoType AccessInfo;

  inline void finishedRenamed() const {
		const Access *this_(static_cast<const Access*>(this));
    if (!modifiesData(this_->type))
			this_->increaseCurrentVersion();
	}
private:
	// check if lock is needed on dynamic type
	template<int n, bool stop = (n==0)>
	struct ModifiesDataAux {
		static inline bool modifiesData(int type) {
			if (type == n-1)
				return AccessInfo::modifiesData(typename AccessInfo::template AccessTag<n-1>());
			else
				return ModifiesDataAux<n-1>::modifiesData(type);
		}
	};
	template<int n>
	struct ModifiesDataAux<n, true> {
		static inline bool modifiesData(int) { return false; }
	};
	inline static bool modifiesData(int type) {
		return ModifiesDataAux<AccessInfo::numAccesses>::modifiesData(type);
  }
};

// ============================================================================
// Option Lockable
// ============================================================================
template<typename Options, bool Lockable>
class Access_Lockable {};

template<typename Options>
class Access_Lockable<Options, true> {
public:
  typedef typename Options::AccessType Access;
  typedef typename Options::AccessInfoType AccessInfo;

  // Used to determine locking order
	inline bool operator<(const Options &rhs) const {
		const Access *this_(static_cast<const Access *>(this));
		assert(this_->handle != 0);
		assert(rhs.handle != 0);
		return (this_->handle < rhs.handle);
	}

  // Check if lock is available, or add a listener
	inline bool getLockOrNotify(LockListener<Options> *task) const {
		const Access *this_(static_cast<const Access *>(this));
    if (!needsLock(this_->type))
      return true;

    return this_->handle->getLockOrNotify(task);
	}

  // Get lock if its free, or return false.
  // Low level interface to lock several objects simultaneously
	inline bool getLock() const {
		const Access *this_(static_cast<const Access *>(this));
    if (!needsLock(this_->type))
  		return true;

    return this_->handle->getLock();
	}

	inline void releaseLock() const {
		const Access *this_(static_cast<const Access *>(this));
    if (!this_->needsLock(this_->type))
      return;
    this_->handle->releaseLock();
	}

private:
	// check if lock is needed on dynamic type
	template<int n, bool stop = (n==0)>
	struct NeedsLockAux {
		static inline bool needsLock(int type) {
			if (type == n-1)
				return AccessInfo::needsLock(typename AccessInfo::template AccessTag<n-1>());
			else
				return NeedsLockAux<n-1>::needsLock(type);
		}
	};
	template<int n>
	struct NeedsLockAux<n, true> {
		static inline bool needsLock(int) { return false; }
	};
	
public:
  inline static bool needsLock(int type) {
		return NeedsLockAux<AccessInfo::numAccesses>::needsLock(type);
  }
};

// ============================================================================
// Access
// ============================================================================
// Wraps up the selection of which action to perform depending on access type
template<typename Options>
class Access
	: public Access_Renaming<Options, Options::Renaming>,
    public Access_Lockable<Options, Options::Lockable>
{
public:
  typedef typename Options::IHandleType IHandle;
  typedef typename Options::AccessInfoType AccessInfo;

  // The handle to the object
  IHandle *handle;
  // Required version
  size_t requiredVersion;
  // Type of access
	int type;

  // Constructors
  Access(const Access &d)
    : handle(d.handle), requiredVersion(d.requiredVersion), type(d.type) {}
	Access(int type_, IHandle *handle_)
    : handle(handle_), requiredVersion(handle_->schedulerGetRequiredVersion(type_)), type(type_) 
	{}
	Access(int type_, IHandle *handle_, size_t version_)
    : handle(handle_), requiredVersion(version_), type(type_) {
	}

	inline IHandle *getHandle() const {
		assert(handle != 0);
		return handle;
	}

	// for debugging
	inline size_t getRequiredVersion() const { return requiredVersion; }
	
	// for debugging
  inline size_t getAvailableVersion() const { return handle->getCurrentVersion(); }

  // Check if version requirement is satisfied, or add a listener
  inline bool isVersionAvailableOrNotify(VersionListener<Options> *task) {
    return handle->isVersionAvailableOrNotify(task, requiredVersion);
  }

  // Increase next required version when task is added
	inline void schedulerIncrease() {
    handle->schedulerIncrease(type);
	}

	inline void increaseCurrentVersion() const {
		handle->increaseCurrentVersion();
	}
};

#endif // __ACCESS_HPP__
