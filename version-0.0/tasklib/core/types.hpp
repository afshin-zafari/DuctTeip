#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include <vector>
#include <deque>
#include <map>

template<typename Options> class TaskBase;
template<typename Options> class TaskQueueUnsafe;

template<typename Options>
struct Types {
    template<typename T>
    struct vector_t {
        typedef typename std::vector<T, typename Options::template Alloc<T>::type > type;
    };
    template<typename T>
    struct deque_t {
        typedef typename std::deque<T, typename Options::template Alloc<T>::type > type;
    };
    template <typename Key, typename Value>
    struct map_t {
        typedef std::map<Key, Value, std::less<Key>,
                typename Options::template Alloc< std::pair<const Key, Value> >::type> type;
    };
//    struct mappair {
//        TaskBase<Options> *root;
//        TaskBase<Options> *last;
//    };
    //typedef typename std::pair<typename Options::version_t, TaskBase<Options> *> taskverpair_t;
    //typedef typename vector_t<taskverpair_t>::type taskheap_t;
    typedef typename Options::version_t version_t;
    typedef typename map_t<version_t, TaskQueueUnsafe<Options> >::type versionmap_t;
//    typedef typename map_t<typename Options::version_t, TaskBase<Options> *>::type versionmap_t;
    typedef typename vector_t<TaskBase<Options> *>::type taskvector_t;
    typedef typename deque_t<TaskBase<Options> *>::type taskdeque_t;
    typedef typename vector_t<int>::type intvector_t;
};

#endif // __TYPES_HPP__
