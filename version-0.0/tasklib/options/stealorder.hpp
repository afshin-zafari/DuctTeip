#ifndef __STEALORDER_HPP__
#define __STEALORDER_HPP__

#include <cstddef>

template<typename Options> class ThreadManager;
template<typename Options> class TaskBase;
template<typename Options> class TaskQueue;

template<typename Options>
class DefaultStealOrder {
public:
    // try to steal a task
    //   id   -- current thread
    //   dest -- stolen task
    // returns true if a task was successfully stolen, false otherwise
    static bool steal(ThreadManager<Options> &tm, size_t id, TaskBase<Options> *&dest) {
        TaskQueue<Options> **taskQueues(tm.getTaskQueues());
        const size_t numQueues = tm.getNumQueues();
        size_t startID = id;
        for (size_t i = id+1; i < numQueues; ++i) {
            if (taskQueues[i]->steal(dest))
                return true;
        }
        for (size_t i = 0; i < startID; ++i) {
            if (taskQueues[i]->steal(dest))
                return true;
        }
        return false;
    }
};

#endif //  __STEALORDER_HPP__
