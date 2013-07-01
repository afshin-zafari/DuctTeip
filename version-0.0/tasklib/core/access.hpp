#ifndef __ACCESS_HPP__
#define __ACCESS_HPP__

#include <algorithm>
#include <cstring>

template<typename Options> class Access;
template<typename Options> class Handle;
template<typename Options> class TaskBase;
template<typename Options> class TaskExecutor;

namespace detail {

// ============================================================================
// Option Contributions
// ============================================================================
template<typename Options, typename T = void>
class Access_Contributions {
public:
    static bool useContrib() { return false; }
    void increaseCurrentVersion(TaskExecutor<Options> &taskExecutor) const {
        const Access<Options> *this_(static_cast<const Access<Options>*>(this));
        this_->getHandle()->increaseCurrentVersion(taskExecutor);
    }
};

template<typename Options>
class Access_Contributions<Options, typename Options::Contributions> {
private:
    bool useContribFlag;
public:
    typedef typename Options::ContributionType Contribution;

    Access_Contributions() : useContribFlag(false) {}
    void setUseContrib(bool value) { useContribFlag = value; }
    bool useContrib() const { return useContribFlag; }
    void addContribution(Contribution c) {
        this->getHandle()->addContribution(c);
    }
    void increaseCurrentVersion(TaskExecutor<Options> &taskExecutor) const {
        const Access<Options> *this_(static_cast<const Access<Options> *>(this));
        if (Access_Contributions<Options>::useContrib())
            this_->getHandle()->increaseCurrentVersionNoUnlock(taskExecutor);
        else
            this_->getHandle()->increaseCurrentVersion(taskExecutor);
    }
};

// ============================================================================
// Option Lockable
// ============================================================================
template<typename Options, typename T = void>
class Access_Lockable {};

template<typename Options>
class Access_Lockable<Options, typename Options::Lockable> {
private:
    bool lockNeeded;
public:
    Access_Lockable() : lockNeeded(false) {}

    // Used to determine locking order
    bool operator<(const Access<Options> &rhs) const {
        const Access<Options> *this_(static_cast<const Access<Options> *>(this));
        return (this_->handle < rhs.handle);
    }

    // Check if lock is available, or add a listener
    bool getLockOrNotify(TaskBase<Options> *task) const {
        if (!lockNeeded)
            return true;

        const Access<Options> *this_(static_cast<const Access<Options> *>(this));
        return this_->handle->getLockOrNotify(task);
    }

    // Get lock if its free, or return false.
    // Low level interface to lock several objects simultaneously
    bool getLock() const {
        if (!lockNeeded)
            return true;

        const Access<Options> *this_(static_cast<const Access<Options> *>(this));
        return this_->handle->getLock();
    }

    void releaseLock(TaskExecutor<Options> &taskExecutor) const {
        if (!lockNeeded)
            return;

        const Access<Options> *this_(static_cast<const Access<Options> *>(this));
        this_->handle->releaseLock(taskExecutor);
    }

public:
    void setNeedsLock(bool needsLock_) { lockNeeded = needsLock_; }
    bool needsLock() const { return lockNeeded; }
};

} // namespace detail

// ============================================================================
// Access
// ============================================================================
// Wraps up the selection of which action to perform depending on access type
template<typename Options>
class Access
  : public detail::Access_Lockable<Options>,
    public detail::Access_Contributions<Options>
{
public:
    typedef typename Options::AccessInfoType AccessInfo;

    // The handle to the object
    Handle<Options> *handle;
    // Required version
    typename Options::version_t requiredVersion;

    // Constructors
    Access() {}
    Access(Handle<Options> *handle_, typename Options::version_t version_)
    : handle(handle_), requiredVersion(version_) {}

    Handle<Options> *getHandle() const {
        return handle;
    }

    // for debugging
    typename Options::version_t getRequiredVersion() const {
        return requiredVersion;
    }

    // Check if version requirement is satisfied, or add a listener
    bool isVersionAvailableOrNotify(TaskBase<Options> *task) {
        return handle->isVersionAvailableOrNotify(task, requiredVersion);
    }
};

#endif // __ACCESS_HPP__
