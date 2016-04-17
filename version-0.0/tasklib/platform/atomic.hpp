#ifndef __ATOMIC_HPP__
#define __ATOMIC_HPP__

#if defined(_MSC_VER)
#define NOMINMAX
#include <windows.h>
#endif
#if defined(__SUNPRO_CC)
#include <atomic.h>
#endif
#if defined(__GNUC__)
#include <pthread.h>
#endif


namespace detail {

#if defined(__SUNPRO_CC)

template<int n> struct AtomicImplAux {};

template<> struct AtomicImplAux<4> {
    template<typename T> static void increase(T *ptr) { atomic_inc_32(ptr); }
    template<typename T> static void decrease(T *ptr) { atomic_dec_32(ptr); }
    template<typename T> static T increase_nv(T *ptr) { return atomic_inc_32_nv(ptr); }
    template<typename T> static T decrease_nv(T *ptr) { return atomic_dec_32_nv(ptr); }
    template<typename T> static T cas(volatile T *ptr, T oldval, T newval) { return atomic_cas_32(ptr, oldval, newval); }
    template<typename T> static void clear(volatile T *ptr) { atomic_and_32(ptr, 0); }
};

#if defined(_INT64_TYPE)
template<> struct AtomicImplAux<8> {
    template<typename T> static void increase(T *ptr) { atomic_inc_64(ptr); }
    template<typename T> static void decrease(T *ptr) { atomic_dec_64(ptr); }
    template<typename T> static T increase_nv(T *ptr) { return atomic_inc_64_nv(ptr); }
    template<typename T> static T decrease_nv(T *ptr) { return atomic_dec_64_nv(ptr); }
    template<typename T> static T cas(volatile T *ptr, T oldval, T newval) { return atomic_cas_64(ptr, oldval, newval); }
    template<typename T> static void clear(volatile T *ptr) { atomic_and_64(ptr, 0); }
};
#endif

struct AtomicImpl {
    template<typename T> static void increase(T *ptr) { AtomicImplAux<sizeof(T)>::increase(ptr); }
    template<typename T> static void decrease(T *ptr) { AtomicImplAux<sizeof(T)>::decrease(ptr); }
    template<typename T> static T increase_nv(T *ptr) { return AtomicImplAux<sizeof(T)>::increase_nv(ptr); }
    template<typename T> static T decrease_nv(T *ptr) { return AtomicImplAux<sizeof(T)>::decrease_nv(ptr); }
    template<typename T> static T cas(volatile T *ptr, T oldval, T newval) { return AtomicImplAux<sizeof(T)>::cas(ptr, oldval, newval); }
    template<typename T> static void clear(volatile T *ptr) { AtomicImplAux<sizeof(T)>::clear(ptr); }

    static void memory_fence_enter() { membar_enter(); }
    static void memory_fence_exit() { membar_exit(); }
    static void memory_fence_producer() { membar_producer(); }
    static void memory_fence_consumer() { membar_consumer(); }

    static bool lock_test_and_set(volatile unsigned int *ptr) {
        if (atomic_swap_32(ptr, 1) == 0) {
            memory_fence_enter();
            return true;
        }
        return false;
    }
    static void lock_release(volatile unsigned int *ptr) {
        memory_fence_exit();
        *ptr = 0;
    }
    static void yield() { sched_yield(); }

    static void rep_nop() { asm  ("rep;nop": : :"memory"); }

};
#endif // __SUNPRO_CC

#if defined(__GNUC__)
struct AtomicImpl {
    template<typename T> static void increase(T *ptr) { __sync_add_and_fetch(ptr, 1); }
    template<typename T> static void decrease(T *ptr) { __sync_sub_and_fetch(ptr, 1); }
    template<typename T> static T increase_nv(T *ptr) { return __sync_add_and_fetch(ptr, 1); }
    template<typename T> static T decrease_nv(T *ptr) { return __sync_sub_and_fetch(ptr, 1); }
    template<typename T> static T cas(volatile T *ptr, T oldval, T newval) { return __sync_val_compare_and_swap(ptr, oldval, newval); }
    template<typename T> static void clear(volatile T *ptr) { __sync_and_and_fetch(ptr, 0); }

#if defined(__SSE2__)
    static void memory_fence_enter() { __builtin_ia32_mfence(); }
    static void memory_fence_exit() { __builtin_ia32_mfence(); }
    static void memory_fence_producer() { __builtin_ia32_mfence(); }
    static void memory_fence_consumer() { __builtin_ia32_mfence(); }
#else
    static void memory_fence_enter() { __asm __volatile ("":::"memory"); }
    static void memory_fence_exit() { __asm __volatile ("":::"memory"); }
    static void memory_fence_producer() { __asm __volatile ("":::"memory"); }
    static void memory_fence_consumer() { __asm __volatile ("":::"memory");}
#endif

    static bool lock_test_and_set(volatile unsigned int *ptr) { return __sync_lock_test_and_set(ptr, 1) == 0; }
    static void lock_release(volatile unsigned int *ptr) { __sync_lock_release(ptr); }

    static void yield() { sched_yield(); }

    static void rep_nop() { __asm __volatile ("rep;nop": : :"memory"); }
};
#endif // __GNUC__


#if defined(_MSC_VER)
struct AtomicImpl {
    template<typename T> static void increase(T *ptr) { InterlockedIncrement(ptr); }
    template<typename T> static void decrease(T *ptr) { InterlockedDecrement(ptr); }
    template<typename T> static T increase_nv(T *ptr) { return InterlockedIncrement(ptr); }
    template<typename T> static T decrease_nv(T *ptr) { return InterlockedDecrement(ptr); }
    template<typename T> static T cas(volatile T *ptr, T oldval, T newval) { NOT_YET_IMPLEMENTED; }

    static void memory_fence_enter() { MemoryBarrier(); }
    static void memory_fence_exit() { MemoryBarrier(); }
    static void memory_fence_producer() { MemoryBarrier(); }
    static void memory_fence_consumer() { MemoryBarrier(); }

};
#endif // _MSC_VER

} // namespace detail

struct Atomic {
    template<typename T> static void increase(T *ptr) { detail::AtomicImpl::increase(ptr); }
    template<typename T> static void decrease(T *ptr) { detail::AtomicImpl::decrease(ptr); }
    template<typename T> static T increase_nv(T *ptr) { return detail::AtomicImpl::increase_nv(ptr); }
    template<typename T> static T decrease_nv(T *ptr) { return detail::AtomicImpl::decrease_nv(ptr); }
    template<typename T> static T cas(volatile T *ptr, T oldval, T newval) { return detail::AtomicImpl::cas(ptr, oldval, newval); }
    template<typename T> static T swap(volatile T *ptr, T newval) {
        T prev;
        T oldval = 0;
        do {
            prev = oldval;
            oldval = detail::AtomicImpl::cas(ptr, prev, newval);
        } while (oldval != prev);
        return oldval;
    }
    template<typename T> static void clear(volatile T *ptr) { detail::AtomicImpl::clear(ptr); }

    // test to grab a lock (true = success, you now own the lock), and perform the required memory barriers
    static bool lock_test_and_set(volatile unsigned int *ptr) { return detail::AtomicImpl::lock_test_and_set(ptr); }

    // release lock grabbed by lock_test_and_set, and perform the required memory barriers
    static void lock_release(volatile unsigned int *ptr) { return detail::AtomicImpl::lock_release(ptr); }

    static void memory_fence_enter() { detail::AtomicImpl::memory_fence_enter(); }
    static void memory_fence_exit() { detail::AtomicImpl::memory_fence_exit(); }
    static void memory_fence_producer() { detail::AtomicImpl::memory_fence_producer(); }
    static void memory_fence_consumer() { detail::AtomicImpl::memory_fence_consumer(); }
    static void yield() { detail::AtomicImpl::yield(); }
    static void rep_nop() { detail::AtomicImpl::rep_nop(); }
};

#endif // __ATOMIC_HPP__
