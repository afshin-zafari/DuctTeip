#ifndef __HANDLE_HPP__
#define __HANDLE_HPP__

#include "core/types.hpp"
#include "core/versionqueue.hpp"
#include "platform/spinlock.hpp"
#include "platform/atomic.hpp"
#include "core/log.hpp"
#include <cassert>

template<typename Options> class Handle;
template<typename Options> class TaskBase;
template<typename Options> class TaskExecutor;
template<typename Options> class SchedulerVersion;
template<typename Options> class Log;

namespace detail {

// ============================================================================
// Option: Handle_Name
// ============================================================================
template<typename Options, typename T = void>
class Handle_Name {};

template<typename Options>
class Handle_Name<Options, typename Options::HandleName> {
private:
    std::string name;
public:
    void setName(const char *name_) { name = name_; }
    std::string getName() const { return name; }
};

// ============================================================================
// Option: Global ID for each handle
// ============================================================================
template<typename Options, typename T = void>
class Handle_GlobalId {};

template<typename Options>
class Handle_GlobalId<Options, typename Options::HandleId> {
private:
    size_t id;
public:

    Handle_GlobalId() {
        static size_t global_handle_id = 0;
        id = Atomic::increase_nv(&global_handle_id);
    }

    size_t getGlobalId() const { return id; }
};

// ============================================================================
// Option: Contributions
// ============================================================================
template<typename Options, typename T = void>
class Handle_Contributions {
public:
    static size_t applyContribution() { return 0; }
};

template<typename Options>
class Handle_Contributions<Options, typename Options::ContributionsLocks>
{
    typedef typename Options::ContributionType Contribution;
private:
    Contribution contrib;
    SpinLock lockContrib;

public:
    Handle_Contributions() {}

    // run by contrib-supporting task when kernel is finished
    // i.e. may be run from other threads at any time
    // destination may be unlocked (was locked when contribution created,
    // but  lock may have been released since then)
    // lock, (unlock, lock)*, createContrib, (unlock, lock)*, [unlock]
    void addContribution(Contribution c) {
        for (;;) {
            SpinLockScopedReleasable contribLock(lockContrib);
            if (contrib.isNull()) {
                // no contrib -- add
                contrib = c;
                return;
            }
            else {
                // merge contribs
                Contribution local;
                local = contrib;
                contrib.setNull();
                contribLock.release();
                c.merge(&local);
            }
        }
    }

    void applyOldContributionsBeforeRead() {
        SpinLockScoped contribLock(lockContrib);
        if (contrib.isNull())
            return;
        contrib.apply();
        contrib.setNull();
    }

    // for debug only, not thread safe.
    bool hasContrib() {
        return !contrib.isNull();
    }
};

template<typename Options>
class Handle_Contributions<Options, typename Options::Contributions>
{
    typedef typename Options::ContributionType Contribution;
private:
    Contribution *contrib;

public:
    Handle_Contributions() : contrib(0) {}

    // run by contrib-supporting task when kernel is finished
    // i.e. may be run from other threads at any time
    // destination may be unlocked (was locked when contribution created,
    // but  lock may have been released since then)
    // lock, (unlock, lock)*, createContrib, (unlock, lock)*, [unlock]
    void addContribution(Contribution *c) {
        for (;;) {
            // expect zero
            Contribution *old = Atomic::cas(&contrib, (Contribution *) 0, c);
            if (old == 0)
                return;

            // get old, write zero
            old = Atomic::swap(&contrib, (Contribution *) 0);
            if (old != 0)
                c->merge(old);
        }
    }

    void applyOldContributionsBeforeRead() {
        Contribution *c = Atomic::swap(&contrib, (Contribution *) 0);
        if (c == 0)
            return;
        Contribution::applyAndFree(c);
    }

    // Return current contribution, or 0 if no contribution is attached
    // Use to reuse an old buffer instead of creating a new one
    Contribution *getContribution() {
        return Atomic::swap(&contrib, (Contribution *) 0);
    }

    // for debug only, not thread safe.
    bool hasContrib() {
        return contrib != 0;
    }
};

// ============================================================================
// LockQueue
// ============================================================================
template<typename Options, typename T = void>
class Handle_LockQueue {
public:
    typename Types<Options>::taskvector_t lockListeners;

    bool wakeLockQueue(TaskExecutor<Options> &taskExecutor) {
        Handle<Options> *this_(static_cast<Handle<Options> *>(this));
        SpinLockScopedReleasable listenerListLock(this_->lockListenerSpinLock);
        if (lockListeners.empty())
            return false;

        typename Types<Options>::taskvector_t wake;
        std::swap(wake, lockListeners);
        listenerListLock.release();
        // ### TODO: Only wake a single task ?
        taskExecutor.getTaskQueue().push_front(&wake[0], wake.size());
        return true;
    }

    void addLockListener(TaskBase<Options> *task) {
        lockListeners.push_back(task);
    }
};

template<typename Options, typename T = void> class Handle_Lockable;

template<typename Options>
class Handle_LockQueue<Options, typename Options::ListQueue>
{
public:
    // lock listeners (on current version)
    TaskBase<Options> *lockListeners;

    Handle_LockQueue() : lockListeners(0) {}

    bool wakeLockQueue(TaskExecutor<Options> &taskExecutor) {
        SpinLockScopedReleasable listenerListLock((Handle_Lockable<Options>*)(this)->lockListenerSpinLock);

        if (lockListeners == 0)
            return false;

        // we can do this atomically if producer only adds to start of list
        TaskBase<Options> *wake = lockListeners;
        lockListeners = 0;
        listenerListLock.release();
        // ### TODO: Only wake a single task?
        taskExecutor.getTaskQueue().push_front_list(wake);
        return true;
    }

    void addLockListener(TaskBase<Options> *task) {
        task->next = lockListeners; // NOT IMPLEMENTED YET!
        lockListeners = task;
    }
};

// ============================================================================
// Option: Lockable
// ============================================================================
template<typename Options, typename T>
class Handle_Lockable {
public:
    void increaseCurrentVersion(TaskExecutor<Options> &taskExecutor) {
        Handle<Options> *this_(static_cast<Handle<Options> *>(this));
        this_->increaseCurrentVersionImpl();
        this_->versionQueue.notifyVersionListeners(taskExecutor, this_->version);
    }
};

template<typename Options>
class Handle_Lockable<Options, typename Options::Lockable>
  : public Handle_Contributions<Options>,
    public Handle_LockQueue<Options>
{
    template<typename, typename> friend class Handle_Contributions;
    template<typename, typename> friend class Handle_LockQueue;
private:
    // reading the listener list requires this lock
    SpinLock lockListenerSpinLock;
    // If object is locked or not
    SpinLockAtomic dataLock;

    // Notify lock listeners when the lock is released
    void notifyLockListeners(TaskExecutor<Options> &taskExecutor) {

        Log<Options>::push(LogTag::LockListeners());
        if (!wakeLockQueue(taskExecutor)) {
            Log<Options>::pop(LogTag::LockListeners(), "locklisteners-empty");
            return;
        }

        taskExecutor.getThreadManager().signalNewWork();
        Log<Options>::pop(LogTag::LockListeners(), "locklisteners-woken", 0);
    }

public:
    Handle_Lockable() {}

    bool getLock() {
        return dataLock.try_lock();
    }

    bool getLockOrNotify(TaskBase<Options> *task) {
        /// ### CHECK FIRST
        SpinLockScoped scopedLock(lockListenerSpinLock);
        if (dataLock.try_lock())
            return true;
        // (make sure lock is not released here, or our listener is never woken.)
        addLockListener(task);

        return false;
    }

    void releaseLock(TaskExecutor<Options> &taskExecutor) {
        {
            SpinLockScoped scopedLock(lockListenerSpinLock); // ### COULD NOT BE NEEDED?
            dataLock.unlock();
        }
        notifyLockListeners(taskExecutor);
    }

    // for contributions that haven't actually got the lock
    void increaseCurrentVersionNoUnlock(TaskExecutor<Options> &taskExecutor) {
        Handle<Options> *this_(static_cast<Handle<Options> *>(this));

        this_->increaseCurrentVersionImpl();
        this_->versionQueue.notifyVersionListeners(taskExecutor, this_->getCurrentVersion());
    }

    void increaseCurrentVersion(TaskExecutor<Options> &taskExecutor) {
        Handle<Options> *this_(static_cast<Handle<Options> *>(this));

        // first check if we owned the lock before version is increased
        // (otherwise someone else might grab the lock in between)
        if (dataLock.is_locked()) {
            {
                SpinLockScoped scopedLock(lockListenerSpinLock); // ### COULD NOT BE NEEDED
                dataLock.unlock();
            }
            // (someone else can snatch the lock here. that is ok.)
            Atomic::increase(&this_->version);
            this_->versionQueue.notifyVersionListeners(taskExecutor, this_->version);
            notifyLockListeners(taskExecutor);
        }
        else {
            Atomic::increase(&this_->version);
            this_->versionQueue.notifyVersionListeners(taskExecutor, this_->version);
        }
    }
};

// ============================================================================
// Option: SubTasks
// ============================================================================
template<typename Options, typename T = void>
class Handle_SubTasks {
    typedef typename Options::version_t version_t;
public:

    // not thread safe
    // if subtasks are not allowed, only master thread can add tasks,
    // and there is no need for synchronization
    version_t schedule(int type) {
        Handle<Options> *this_(static_cast<Handle<Options> *>(this));
        return this_->requiredVersion.schedule(type);
    }
};

template<typename Options>
class Handle_SubTasks<Options, typename Options::SubTasks> {
    typedef typename Options::version_t version_t;
private:
    SpinLock spinlock; // can this be removed ?
    // might be enough with atomic increase.
    // if requiredversion is updated by several threads concurrently, this means
    // that several threads are adding a task to update a handle concurrently,
    // which is already a datarace, and no order can be promised.
    // read-read or add-add or write-write should be ok.
public:

    version_t schedule(int type) {
        Handle<Options> *this_(static_cast<Handle<Options> *>(this));
        SpinLockScoped lock(spinlock);
        return this_->requiredVersion.schedule(type);
    }
};

} // namespace detail

// ============================================================================
// Handle
// ============================================================================
template<typename Options>
class Handle_
  : public detail::Handle_Lockable<Options>,
    public detail::Handle_GlobalId<Options>,
    public detail::Handle_SubTasks<Options>,
    public detail::Handle_Name<Options>
{
    template<typename, typename> friend class Handle_Lockable;
    template<typename, typename> friend class Handle_Contributions;
    template<typename, typename> friend class Handle_SubTasks;

    typedef typename Options::version_t version_t;

private:

public:

    version_t version;

    VersionQueue<Options> versionQueue;

    // next required version for each access type
    SchedulerVersion<Options> requiredVersion;

    // Not copyable -- there must be only one data handle per data.
    Handle_(const Handle_ &);
    const Handle_ &operator=(const Handle_ &);

    void increaseCurrentVersionImpl() {
        Atomic::increase(&version);
    }

public:
    Handle_() : version(0) {}
    ~Handle_() { return;
        if (version != requiredVersion.nextVersion()-1) {
            version_t rec = version;
            std::cerr<<"ver=" << version << " req=" << requiredVersion.nextVersion()-1 << std::endl;
            while (version != requiredVersion.nextVersion()-1) {
                Atomic::memory_fence_consumer();
                if (rec != version) {
                    std::cerr<<"ver=" << version << " req=" << requiredVersion.nextVersion()-1 << std::endl;
                    rec = version;
                }
            }
//            raise(SIGINT);
//            assert(version == requiredVersion.nextVersion()-1);
//            Atomic::memory_fence_consumer();
        }
        assert(version == requiredVersion.nextVersion()-1);
    }

    version_t getCurrentVersion() { return version; }

    bool isVersionAvailableOrNotify(TaskBase<Options> *listener, version_t requiredVersion_) {
    
        // check if required version is available
        if ( (int) (version - requiredVersion_) >= 0)
            return true;

        versionQueue.addVersionListener(listener, requiredVersion_);
        return false;
    }

}; // __attribute__((aligned(64)));

template<typename Options> class Handle : public Options::HandleType {};

#endif // __HANDLE_HPP__
