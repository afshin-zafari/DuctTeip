#ifndef __PRIOSCHEDULER_HPP__
#define __PRIOSCHEDULER_HPP__

#include <cstddef>

template<typename Options, unsigned int N> class Task;

template<typename Options>
class PrioScheduler {
public:
    typedef typename Options::TaskPriorities requires_TaskPriorities;
    typedef typename Types<Options>::taskdeque_t taskdeque_t;

public:
    static void getTask(taskdeque_t &taskQueue, TaskBase<Options> * &elem) {
        // TODO: Uses linear search, which is unnecessarily slow. It may be faster to keep two task queues instead.
        size_t pos = 0;
        int maxprio = 0;
        const size_t size = taskQueue.size();
        for (size_t i = 0; i < size; ++i) {
            if (taskQueue[i]->getPriority() > maxprio) {
                maxprio = taskQueue[i]->getPriority();
                pos = i;
            }
        }

        elem = taskQueue[pos];
        // could swap with last element and erase last, which is quicker but destroys order
        //taskQueue[pos] = taskQueue[size-1];
        //taskQueue.erase(taskQueue.begin() + (size-1));
        // instead, just remove it and move all other elements:
        taskQueue.erase(taskQueue.begin() + pos);
    }

    static bool stealTask(taskdeque_t &taskQueue, TaskBase<Options> * &elem) {
        getTask(taskQueue, elem);
        return true;
    }

    // initial placement of task
    //   cpuid -- placement supplied by user (or 0)
    // returns which thread should execute the task
    static size_t place(TaskBase<Options> *task, int cpuid, size_t numThreads) {
        return cpuid % numThreads;
    }

};

#endif // __PRIOSCHEDULER_HPP__

