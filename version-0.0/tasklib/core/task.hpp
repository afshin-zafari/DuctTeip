#ifndef __TASK_HPP__
#define __TASK_HPP__

// todo: support fixed-access tasks?
//   vector of accesses could be fixed

#include "core/types.hpp"
#include "core/accessutil.hpp"
#include "platform/atomic.hpp"
#include <string>

template<typename Options> class Access;
template<typename Options> class Handle;
template<typename Options> class TaskBase;
template<typename Options> class TaskExecutor;

namespace detail {

// ============================================================================
// Option: ReusableTasks
// ============================================================================
template<typename Options, typename T = void>
struct Task_ReusableTasks {
    bool isReusable() const { return false; }
};

template<typename Options>
struct Task_ReusableTasks<Options, typename Options::ReusableTasks> {
    virtual bool isReusable() const = 0;
};

// ============================================================================
// Option: StealableFlag
// ============================================================================
template<typename Options, typename T = void>
struct Task_StealableFlag {};

template<typename Options>
struct Task_StealableFlag<Options, typename Options::StealableFlag> {
    virtual bool isStealable() const = 0;
};

// ============================================================================
// Option: TaskPriorities
// ============================================================================
template<typename Options, typename T = void>
struct Task_Priorities {};

template<typename Options>
struct Task_Priorities<Options, typename Options::TaskPriorities> {
    virtual int getPriority() const = 0;
};

// ============================================================================
// Option: Global ID for each task
// ============================================================================
template<typename Options, typename T = void>
class Task_GlobalId {};

template<typename Options>
class Task_GlobalId<Options, typename Options::TaskId> {
private:
    size_t id;
public:
    static size_t global_task_id;
    Task_GlobalId() {
        id = Atomic::increase_nv(&global_task_id);
    }
    size_t getGlobalId() const { return id; }
};

template<typename Options>
size_t Task_GlobalId<Options, typename Options::TaskId>::global_task_id;

// ============================================================================
// Option Contributions
// ============================================================================
template<typename Options, typename T = void>
class Task_Contributions {
public:
    static bool runContrib() { return false; }
    static void applyOldContributionsBeforeRead() {}
};
template<typename Options>
class Task_Contributions<Options, typename Options::Contributions> {
public:
    virtual bool canRunWithContribs() { return false; }
    bool runContrib() {
        TaskBase<Options> *this_(static_cast<TaskBase<Options> *>(this));
        if (!this_->canRunWithContribs())
            return false;

        const size_t numAccess = this_->getNumAccess();
        Access<Options> *access(this_->getAccess());
        for (size_t i = 0; i < numAccess; ++i) {
            if (!access[i].getLock())
                access[i].setUseContrib(true);
        }
        return true;
    }
    void applyOldContributionsBeforeRead() {
        TaskBase<Options> *this_(static_cast<TaskBase<Options> *>(this));
        const size_t numAccess = this_->getNumAccess();
        Access<Options> *access(this_->getAccess());
        for (size_t i = 0; i < numAccess; ++i) {
            if (!access[i].needsLock())
                access[i].getHandle()->applyOldContributionsBeforeRead();
        }
    }
};

// ============================================================================
// Option PassTaskExecutor
// ============================================================================
template<typename Options, typename T = void>
struct Task_PassTaskExecutor {
    virtual void run() = 0;
};

template<typename Options>
struct Task_PassTaskExecutor<Options, typename Options::PassTaskExecutor> {
    virtual void run(TaskExecutor<Options> *) = 0;
};

// ============================================================================
// Option TaskName
// ============================================================================
template<typename Options, typename T = void>
struct Task_TaskName {};

template<typename Options>
struct Task_TaskName<Options, typename Options::TaskName> {
    virtual std::string getName() = 0;
};

// ============================================================================
// Option StolenFlag
// ============================================================================
template<typename Options, typename T = void>
class Task_StolenFlag {};
template<typename Options>
class Task_StolenFlag<Options, typename Options::StolenFlag>
{
private:
    bool stolen;

public:
    Task_StolenFlag() : stolen(false) {}
    void setStolen(bool isStolen) { stolen = isStolen; }
    bool isStolen() { return stolen; }
};

// ============================================================================
// Option PreferredThread
// ============================================================================
template<typename Options, typename T = void>
class Task_PreferredThread {
public:
    int getPreferredThread() const { return 0; }
};

template <typename Options>
class Task_PreferredThread<Options, typename Options::PreferredThread>
{
private:
    int preferredThread;
public:
    Task_PreferredThread() : preferredThread(0) {}

    // called from ThreadManager when a subtask is added
    void setPreferredThread(int threadId) {
        preferredThread = threadId;
    }
    int getPreferredThread() const {
        return preferredThread;
    }
};

// ============================================================================
// Option Lockable
// ============================================================================
template<typename Options, typename T = void>
struct Task_Lockable {
    static bool tryLock(TaskExecutor<Options> &) { return true; }
    static void setLockable(Access<Options> &, int) {}
};
template<typename Options>
struct Task_Lockable<Options, typename Options::Lockable> {
    bool tryLock(TaskExecutor<Options> &taskExecutor) {
        TaskBase<Options> *this_(static_cast<TaskBase<Options> *>(this));
        const size_t numAccess = this_->getNumAccess();
        Access<Options> *access(this_->getAccess());
        for (size_t i = 0; i < numAccess; ++i) {
            if (!access[i].getLockOrNotify(this_)) {
                for (size_t j = 0; j < i; ++j)
                    access[i-j-1].releaseLock(taskExecutor);
                return false;
            }
        }
        return true;
    }

    void setLockable(Access<Options> &access, int type) {
        if (AccessUtil<Options>::needsLock(type))
            access.setNeedsLock(true);
    }
};

// ============================================================================
// Option ListQueue
// ============================================================================
template<typename Options, typename T = void>
class Task_ListQueue {};

template<typename Options>
class Task_ListQueue<Options, typename Options::ListQueue> {
public:
    std::ptrdiff_t nextPrev;
    Task_ListQueue() : nextPrev(0) {}
};


} // namespace detail

// ============================================================================
// TaskBase_ : Base for tasks without dependencies
// ============================================================================
template<typename Options>
class TaskBase_
  : public detail::Task_PassTaskExecutor<Options>,
    public detail::Task_GlobalId<Options>,
    public detail::Task_StolenFlag<Options>,
    public detail::Task_PreferredThread<Options>,
    public detail::Task_TaskName<Options>,
    public detail::Task_Priorities<Options>,
    public detail::Task_StealableFlag<Options>,
    public detail::Task_ReusableTasks<Options>,
    public detail::Task_Contributions<Options>,
    public detail::Task_Lockable<Options>
{
    template<typename, typename> friend class Task_PassThreadId;
    template<typename, typename> friend struct Task_AccessData;

protected:
    size_t numAccess;
    size_t accessIdx;
    Access<Options> *accessPtr;

    TaskBase_(size_t numAccess_, Access<Options> *accessPtr_)
     : numAccess(numAccess_), accessIdx(0), accessPtr(accessPtr_) {}

public:
    TaskBase_() : numAccess(0), accessIdx(0), accessPtr(0) {}
    virtual ~TaskBase_() {}

    size_t getNumAccess() const { return numAccess; }
    Access<Options> *getAccess() { return accessPtr; }
    bool areDependenciesSolvedOrNotify() {
        const size_t numAccess = this->getNumAccess();
        for (; accessIdx < numAccess; ++accessIdx) {
            if (!accessPtr[accessIdx].isVersionAvailableOrNotify(static_cast<TaskBase<Options> *>(this))) {
                ++accessIdx; // We consider this dependency fulfilled now, as it will be when we get the callback.
                return false;
            }
        }
        return true;
    }
    void finished(TaskExecutor<Options> &ex) {
        const size_t numAccess = this->getNumAccess();
        Access<Options> *access(this->getAccess());
        for (size_t i = numAccess; i > 0;)
            access[--i].increaseCurrentVersion(ex);
        if (!this->isReusable())
            delete this;
    }
};

// export Options::TaskBaseType as TaskBase (default: TaskBase_<Options>)
template<typename Options> class TaskBase
 : public detail::Task_ListQueue<Options>,
   public Options::TaskBaseType
{};

// ============================================================================
// Task : Tasks with fixed number of dependencies
// ============================================================================
template<typename Options, int N>
class Task_ : public TaskBase<Options> {
    Access<Options> access[N];
    typedef typename Options::AccessInfoType AccessInfo;
    typedef typename AccessInfo::Type AccessType;
public:
    Task_() {
        // If this assignment is done through a constructor, we can save the initial assignment,
        // but then any user-overloaded TaskBase_() class must forward both constructors.
        TaskBase<Options>::numAccess = N;
        TaskBase<Options>::accessPtr = &access[0];
    }
    void registerAccess(size_t i, AccessType type, Handle<Options> *handle) {
        Access<Options> &a(access[i]);
        a = Access<Options>(handle, handle->schedule(type));
        detail::Task_Lockable<Options>::setLockable(a, type);
        Log<Options>::addDependency(static_cast<TaskBase<Options> *>(this), &a, type);
    }
};

template<typename Options>
class Task_<Options, 0> : public TaskBase<Options> {};

// export "Options::TaskType<>::type" (default: Task_) as type Task
template<typename Options, unsigned int N> class Task
 : public Options::template TaskType<N>::type {};

// ============================================================================
// TaskDynamic : Tasks with variable number of dependencies
// ============================================================================
template<typename Options>
class TaskDynamic_ : public TaskBase<Options> {
    typedef typename Options::AccessInfoType AccessInfo;
    typedef typename AccessInfo::Type AccessType;
    typedef typename Types<Options>::template vector_t< Access<Options> >::type access_vector_t;
    typedef typename access_vector_t::reverse_iterator access_ritr;
    access_vector_t access;
public:
    TaskDynamic_() {}

    void registerAccess(AccessType type, Handle<Options> *handle) {
        access.push_back(Access<Options>(handle, handle->schedule(type)));
        Access<Options> &a(access[access.size()-1]);
        detail::Task_Lockable<Options>::setLockable(a, type);
        Log<Options>::addDependency(static_cast<TaskBase<Options> *>(this), &a, type);
        ++TaskBase<Options>::numAccess;
        TaskBase<Options>::accessPtr = &access[0]; // vector may be reallocated at any add
    }
};

// export "Options::TaskDynamicType" (default: TaskDynamic_) as type TaskDynamic
template<typename Options> class TaskDynamic : public Options::TaskDynamicType {};

#endif // __TASK_HPP__
