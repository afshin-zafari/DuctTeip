#ifndef __GETTIME_HPP__
#define __GETTIME_HPP__

namespace Time {

//
// Routines for getting the current time.
//
// Defines:
//   typedef TimeUnit
//   TimeUnit getTime() // returns current time
//   TimeUnit getFreq() // returns time units per second
//

#if defined(_MSC_VER) // MICROSOFT VISUAL C++

#define NOMINMAX
#include <windows.h>
#pragma message("Using timer = Microsoft QueryPerformanceCounter()")
typedef unsigned __int64 TimeUnit;
static inline TimeUnit getTime() {
    LARGE_INTEGER i;
    QueryPerformanceCounter(&i);
    return i.QuadPart;
}

static inline TimeUnit getFreq() {
    LARGE_INTEGER i;
    QueryPerformanceFrequency(&i);
    return i.QuadPart;
}

#else
#include <sys/time.h>
//typedef long int TimeUnit;
//static inline TimeUnit getTime() {
//    timeval tv;
//    gettimeofday(&tv, 0);
//    return (tv.tv_sec*1000000+tv.tv_usec);
//}
typedef unsigned long long TimeUnit;

static inline TimeUnit getTime() {
  unsigned long long tsc;

#if defined(__SUNPRO_CC)
  asm
#else
  __asm__ __volatile__
#endif
    ("rdtsc;"
     "shl $32, %%rdx;"
     "or %%rdx, %%rax;"
    : "=A"(tsc)
    :
    : "%edx"
  );
  return tsc;
}

#endif

class Instrumentation {
private:
    TimeUnit start;
    TimeUnit &counter;
public:
    Instrumentation(TimeUnit &counter_) : counter(counter_) {
        start = getTime();
    }
    ~Instrumentation() {
        TimeUnit stop = getTime();
        counter += (stop - start);
    }
};

}

#endif // __GETTIME_HPP__
