#ifndef __ATOMIC_HPP__
#define __ATOMIC_HPP__

#if defined(_MSC_VER)
#define NOMINMAX
#include <windows.h>
#endif
#if defined(__SUNPRO_CC)
#include <atomic.h>
#endif

// design choice: class with static methods or macros?
//   change to macros if these do not get inlined.

#if defined(__SUNPRO_CC)
template<size_t size> struct AtomicImpl;
template<> struct AtomicImpl<4> {
  template<class T> static inline void atomic_increase(T *ptr) {
    atomic_inc_32(ptr);
  }
  template<class T> static inline T atomic_increase_nv(T *ptr) {
    return atomic_inc_32_nv(ptr);
  }
};
#if defined(_INT64_TYPE)
template<> struct AtomicImpl<8> {
  template<class T> static inline void atomic_increase(T *ptr) {
    atomic_inc_64(ptr);
  }
  template<class T> static inline T atomic_increase_nv(T *ptr) {
    return atomic_inc_64_nv(ptr);
  }
};
#endif
#endif

struct Atomic {
  template<class T> static inline void atomic_increase(T *ptr) {
#if defined(__SUNPRO_CC)
    AtomicImpl<sizeof(T)>::atomic_increase<T>(ptr);
#elif defined(__GNUC__)
    __sync_add_and_fetch(ptr, 1);
#elif defined(_MSC_VER)
    InterlockedIncrement(ptr);
#else
    #error Atomic increase not implemented for this platform.
#endif
  }
  template<class T> static inline T atomic_increase_nv(T *ptr) {
#if defined(__SUNPRO_CC)
    return AtomicImpl<sizeof(T)>::atomic_increase_nv<T>(ptr);
#elif defined(__GNUC__)
    return __sync_add_and_fetch(ptr, 1);
#elif defined(_MSC_VER)
    return InterlockedIncrement(ptr);
#else
    #error Atomic increase not implemented for this platform.
#endif
  }

  static inline void memory_fence() {
#if defined(__GNUC__) && defined(__SSE2__)
    __builtin_ia32_mfence();
#elif defined(_MSC_VER)
    MemoryBarrier();
#elif defined(__SUNPRO_CC)
    asm volatile ("":::"memory");
#else
    __sync_synchronize();
//#else
//#error Memory fence not implemented for this platform.
#endif
  }
};

#endif // __ATOMIC_HPP__
