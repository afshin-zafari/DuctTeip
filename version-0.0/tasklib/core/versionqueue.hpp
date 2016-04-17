#ifndef __VERSIONQUEUE_HPP_
#define __VERSIONQUEUE_HPP_

#include "platform/spinlock.hpp"
#include "platform/atomic.hpp"
#include "core/log.hpp"

// ============================================================================
// Option: VersionQueue
// ============================================================================
template<typename Options, typename T = void>
class VersionQueue {
public:
    typedef typename Types<Options>::taskvector_t task_vector_t;
    typedef typename Types<Options>::template map_t<typename Options::version_t, task_vector_t>::type versionmap_t;
    typedef typename versionmap_t::iterator vermapiter_t;
    typedef typename Options::version_t version_t;

    // lock that must be held during usage of the listener list, and when unlocking
    SpinLock versionListenerSpinLock;
    // version listeners, per version
    versionmap_t versionListeners;

    static void checkDependencies(task_vector_t &list, typename Options::LazyDependencyChecking) {}
    static void checkDependencies(task_vector_t &list, typename Options::EagerDependencyChecking) {
        task_vector_t wokenTasks;
        wokenTasks.reserve(list.size());
        // check listeners for further version dependencies
        for (size_t i = 0; i < list.size(); ++i) {
            if (!list[i]->areDependenciesSolvedOrNotify())
                continue;
            wokenTasks.push_back(list[i]);
        }
        std::swap(wokenTasks, list);
    }

    void notifyVersionListeners(TaskExecutor<Options> &taskExecutor, version_t version) {

        Log<Options>::push(LogTag::VersionListeners());

        for (;;) {
            task_vector_t list;
            {
                SpinLockScoped verListenerLock(versionListenerSpinLock);
                vermapiter_t l(versionListeners.begin());

                // return if there are no version listeners
                if (l == versionListeners.end()) {
                    Log<Options>::pop(LogTag::VersionListeners(), "versionlisteners-empty");
                    return;
                }

                // return if next version listener is for future version
                if (l->first > version) {
                    Log<Options>::pop(LogTag::VersionListeners(), "versionlisteners-future");
                    return;
                }

                // move listener list to local vector and remove the shared copy
                std::swap(list, l->second);
                versionListeners.erase(l);
            }

            // Note that a version is increased while holding a lock, and adding a
            // version listener requires holding the same lock. Hence, it is not
            // possible to add a listener for an old version here.
            // The version number is already increased when we wake tasks.

            // ### handle single-task case here to gain some speed?

            checkDependencies(list, typename Options::DependencyChecking());

            // add woken tasks to ready queue
            if (!list.empty()) {
                taskExecutor.getTaskQueue().push_front(&list[0], list.size());
                taskExecutor.getThreadManager().signalNewWork();
            }
            Log<Options>::pop(LogTag::VersionListeners(), "versionlisteners-woken", list.size());
        }
    }

    void addVersionListener(TaskBase<Options> *task, version_t version) {
        SpinLockScoped listenerLock(versionListenerSpinLock);
        versionListeners[version].push_back(task);
    }

};

template<typename Options>
class VersionQueue<Options, typename Options::ListQueue> {
private:
    typedef typename Types<Options>::versionmap_t versionmap_t;
    typedef typename versionmap_t::iterator vermapiter_t;
    typedef typename Options::version_t version_t;

    // lock that must be held during usage of the listener list, and when unlocking
    SpinLock versionListenerSpinLock;
    // version listeners, per version
    versionmap_t versionListeners;

    struct DependenciesNotSolvedPredicate {
        static bool test(TaskBase<Options> *elem) {
            return !elem->areDependenciesSolvedOrNotify();
        }
    };

    static void checkDependencies(TaskQueueUnsafe<Options> &list, typename Options::LazyDependencyChecking) {}
    static void checkDependencies(TaskQueueUnsafe<Options> &list, typename Options::EagerDependencyChecking) {
        list.erase_if(DependenciesNotSolvedPredicate());
    }

public:
    void notifyVersionListeners(TaskExecutor<Options> &taskExecutor, version_t version) {
        Log<Options>::push(LogTag::VersionListeners());

        for (;;) {
            TaskQueueUnsafe<Options> list;
            {
                SpinLockScoped verListenerLock(versionListenerSpinLock);

                // return if there are no version listeners
                if (versionListeners.empty()) {
                    Log<Options>::pop(LogTag::VersionListeners(), "versionlisteners-empty");
                    return;
                }

                vermapiter_t l(versionListeners.begin());

                // return if next version listener is for future version
                if (l->first > version) {
                    Log<Options>::pop(LogTag::VersionListeners(), "versionlisteners-future");
                    return;
                }

                list = l->second;
                versionListeners.erase(l);
            }

            // Note that a version is increased while holding a lock, and adding a
            // version listener requires holding the same lock. Hence, it is not
            // possible to add a listener for an old version here.
            // The version number is already increased when we wake tasks.

            // ### handle single-task case here to gain some speed?

            // iterate through list and remove elements that are not ready

            checkDependencies(list, typename Options::DependencyChecking());

            if (list.gotWork()) {
                taskExecutor.getTaskQueue().push_front_list(list);
                taskExecutor.getThreadManager().signalNewWork();
            }
            Log<Options>::pop(LogTag::VersionListeners(), "versionlisteners-woken", 0);
        }
    }

    void addVersionListener(TaskBase<Options> *task, version_t version) {
        SpinLockScoped listenerLock(versionListenerSpinLock);
        versionListeners[version].push_back(task);
    }
};

#endif // __VERSIONQUEUE_HPP_
