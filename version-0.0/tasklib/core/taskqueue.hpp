#ifndef __TASKQUEUE_HPP__
#define __TASKQUEUE_HPP__

#include "core/types.hpp"
#include "platform/spinlock.hpp"
#include "core/log.hpp"

template<typename Options> class TaskBase;
template<typename Options> class Log;

template<typename Options, typename T = void>
class TaskQueue_ {
private:
    SpinLock spinlock;
    bool hasWork;
    typename Types<Options>::taskdeque_t buffer;

    TaskQueue_(const TaskQueue_ &);
    const TaskQueue_ &operator=(const TaskQueue_ &);

    template<typename, typename> friend struct detail::Log_DumpState;

public:
    TaskQueue_() : hasWork(false) {}

    void push_back(TaskBase<Options> *elem) {
        SpinLockScoped lock(spinlock);
        hasWork = true;
        buffer.push_back(elem);

        Log<Options>::add(LogTag::TaskQueue(), "push_back", buffer.size());
    }

    void push_front(TaskBase<Options> *elem) {
        SpinLockScoped lock(spinlock);
        hasWork = true;
        buffer.push_front(elem);

        Log<Options>::add(LogTag::TaskQueue(), "push_front", buffer.size());
    }

    void push_front(TaskBase<Options> **elem, size_t len) {
        SpinLockScoped lock(spinlock);
        hasWork = true;
        for (size_t i = len; i != 0; )
            buffer.push_front(elem[--i]);

        Log<Options>::add(LogTag::TaskQueue(), "push_front2", buffer.size());
    }

//    void dump() {
//        SpinLockScoped lock(spinlock);
//        for (size_t i = 0; i < buffer.size(); ++i)
//            std::cerr << buffer[i] << " ";
//        std::cerr << std::endl;
//    }

    bool get(TaskBase<Options> * &elem) {
        SpinLockScoped lock(spinlock);
        if (buffer.empty()) {
            hasWork = false;
            return false;
        }
        Options::Scheduler::getTask(buffer, elem);

        Log<Options>::add(LogTag::TaskQueue(), "get_tasks", buffer.size());
        return true;
    }

    bool steal(TaskBase<Options> * &elem) {
//      // This could be faster, but is it correct?
//        SpinLockTryLock lock(spinlock);
//        if (!lock.success)
//            return false;
        SpinLockScoped lock(spinlock);
        if (buffer.empty()) {
            hasWork = false;
            return false;
        }
        Log<Options>::add(LogTag::TaskQueue(), "steal", buffer.size());
        return Options::Scheduler::stealTask(buffer, elem);
    }

    size_t size() {
        SpinLockScoped lock(spinlock);
        return buffer.size();
    }

    bool gotWork() { return hasWork; }
};

// TaskQueueUnsafe is a (non-thread-safe) doubly linked list
// (or "subtraction linked list", see http://en.wikipedia.org/wiki/XOR_linked_list)
//
// To walk forward in the list: next = prev + curr->nextPrev
//
// Example: List structure and removal of an element.
//   List elements: A, B, C
//   Algorithm pointers: p (previous), c (current), n (next)
//
//        p     c     n
//  0     A     B     C     0
// A-0   B-0   C-A   0-B    C
//
// update prev->nextPrev:
//          p       c     n
//  0       A       B     C     0
// A-0   B-0-B+C   C-A   0-B    C
//
// update next->nextPrev:
//          p       c       n
//  0       A       B       C     0
// A-0   B-0-B+C   C-A   D-B+B-A  C
//
// result:
//        p    n
//  0     A    C    0
// A-0   C-0  D-A   C

template<typename Options>
class TaskQueueUnsafe {
private:
    TaskBase<Options> *first;
    TaskBase<Options> *last;

    template<typename, typename> friend struct detail::Log_DumpState;

public:
    TaskQueueUnsafe() : first(0), last(0) {}

    void push_back(TaskBase<Options> *elem) {
        if (last == 0) {
            elem->nextPrev = 0;
            first = last = elem;
        }
        else {
            last->nextPrev = last->nextPrev + reinterpret_cast<std::ptrdiff_t>(elem);
            elem->nextPrev = -reinterpret_cast<std::ptrdiff_t>(last);
            last = elem;
        }
        Log<Options>::add(LogTag::TaskQueue(), "push_back", 0);
    }

    void push_front(TaskBase<Options> *elem) {
        if (first == 0) {
            elem->nextPrev = 0;
            first = last = elem;
        }
        else {
            first->nextPrev -= reinterpret_cast<std::ptrdiff_t>(elem);
            elem->nextPrev = reinterpret_cast<std::ptrdiff_t>(first);
            first = elem;
        }

        Log<Options>::add(LogTag::TaskQueue(), "push_front", 0);
    }

    // takes ownership of input list
    void push_back_list(TaskQueueUnsafe<Options> &rhs) {
        if (first == 0) {
            first = rhs.first;
            last = rhs.last;
        }
        else {
            last->nextPrev += reinterpret_cast<std::ptrdiff_t>(rhs.first);
            rhs.first->nextPrev -= reinterpret_cast<std::ptrdiff_t>(rhs.last);
            last = rhs.last;
        }
        Log<Options>::add(LogTag::TaskQueue(), "push_back_list", 0);
    }

    // takes ownership of input list
    void push_front_list(TaskQueueUnsafe<Options> &rhs) {
        if (first == 0) {
            first = rhs.first;
            last = rhs.last;
        }
        else {
            first->nextPrev -= reinterpret_cast<std::ptrdiff_t>(rhs.last);
            rhs.last->nextPrev += reinterpret_cast<std::ptrdiff_t>(first);
            first = rhs.first;
        }

        Log<Options>::add(LogTag::TaskQueue(), "push_front_list", 0);
    }

    bool get(TaskBase<Options> * &elem) {
        if (first == 0)
            return false;

        elem = first;
        first = reinterpret_cast<TaskBase<Options> *>(first->nextPrev);
        if (first == 0)
            last = 0;
        else
            first->nextPrev += reinterpret_cast<std::ptrdiff_t>(elem);

        Log<Options>::add(LogTag::TaskQueue(), "get_tasks", 0);
        return true;
    }

    bool steal(TaskBase<Options> * &elem) {
        if (last == 0)
            return false;

        elem = last;
        last = reinterpret_cast<TaskBase<Options> *>(-last->nextPrev);
        if (last == 0)
            first = 0;
        else
            last->nextPrev -= reinterpret_cast<std::ptrdiff_t>(elem);

        Log<Options>::add(LogTag::TaskQueue(), "steal", 0);
        return true;
    }

#ifdef DEBUG_TASKQUEUE
    void dump() {
        if (first == 0) {
            std::cerr << "[Empty]" << std::endl;
            return;
        }
        TaskBase<Options> *curr(first);
        TaskBase<Options> *prev(0);
        TaskBase<Options> *next;

        for (;;) {
            std::cerr << curr << " ";
            next = reinterpret_cast<TaskBase<Options> *>(
                reinterpret_cast<std::ptrdiff_t>(prev) + curr->nextPrev);
            if (next == 0) {
                std::cerr << std::endl;
                return;
            }
            prev = curr;
            curr = next;
        }
    }
#endif // DEBUG_TASKQUEUE

    template<typename Pred>
    void erase_if(Pred) {
        if (first == 0)
            return;
        TaskBase<Options> *prev(0);
        TaskBase<Options> *curr(first);

        for (;;) {
            TaskBase<Options> *next = reinterpret_cast<TaskBase<Options> *>(
                reinterpret_cast<std::ptrdiff_t>(prev) + curr->nextPrev);

            if (Pred::test(curr)) {
                if (prev == 0)
                    first = next;
                else {
                    prev->nextPrev = prev->nextPrev
                        - reinterpret_cast<std::ptrdiff_t>(curr)
                        + reinterpret_cast<std::ptrdiff_t>(next);
                }

                if (next == 0) {
                    last = prev;
                    return;
                }
                else {
                    next->nextPrev = next->nextPrev
                        + reinterpret_cast<std::ptrdiff_t>(curr)
                        - reinterpret_cast<std::ptrdiff_t>(prev);
                }

                prev = 0;
                curr = first;
                continue;

            }
            else {
                prev = curr;
                if (next == 0)
                    return;
            }
            curr = next;
        }
    }

    bool gotWork() { return first != 0; }
};


template<typename Options>
class TaskQueue_<Options, typename Options::ListQueue>
 : public TaskQueueUnsafe<Options> {
private:
    SpinLock spinlock;

    TaskQueue_(const TaskQueue_ &);
    const TaskQueue_ &operator=(const TaskQueue_ &);

    template<typename, typename> friend struct detail::Log_DumpState;

public:
    TaskQueue_() {}

    void push_back(TaskBase<Options> *elem) {
        SpinLockScoped lock(spinlock);
        TaskQueueUnsafe<Options>::push_back(elem);
    }

    void push_front(TaskBase<Options> *elem) {
        SpinLockScoped lock(spinlock);
        TaskQueueUnsafe<Options>::push_front(elem);
    }

    // takes ownership of input list
    void push_back_list(TaskQueueUnsafe<Options> &list) {
        SpinLockScoped lock(spinlock);
        TaskQueueUnsafe<Options>::push_back_list(list);
    }

    // takes ownership of input list
    void push_front_list(TaskQueueUnsafe<Options> &list) {
        SpinLockScoped lock(spinlock);
        TaskQueueUnsafe<Options>::push_front_list(list);
    }

    bool get(TaskBase<Options> * &elem) {
        SpinLockScoped lock(spinlock);
        return TaskQueueUnsafe<Options>::get(elem);
    }

    bool steal(TaskBase<Options> * &elem) {
        SpinLockScoped lock(spinlock);
        return TaskQueueUnsafe<Options>::steal(elem);
    }
};

template<typename Options> class TaskQueue : public TaskQueue_<Options> {};

#endif // __TASKQUEUE_HPP__
