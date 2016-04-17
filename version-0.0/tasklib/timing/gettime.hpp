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

//#include <intrin.h>
//#pragma intrinsic(__rdtsc)
//#pragma message("Using timer = Microsoft __rdtsc()")
//typedef unsigned __int64 TimeUnit;
//static inline TimeUnit getTime() {
//	return __rdtsc();
//}
//static inline TimeUnit getFreq() {
//  // Not true, just make something up to get good order of the numbers
//	return 1000000;
//}

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

//#elif defined(sun) || defined(__sun)
//typedef hrtime_t TimeUnit;
//static inline TimeUnit getTime() {
//	return gethrtime() / 1000000;
//}
//static inline TimeUnit getFreq() {
//  // gethrvtime() returns nanoseconds, so frequency is 1,000,000,000 ns/s
//	return 1000;
//}
//#elif defined(__i386__)
//typedef unsigned long long TimeUnit;
////#warning("Using timer = rdtsc x86")
//static __inline__ TimeUnit getTime() {
//  rdtsc_t x;
//	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
//	return x;
//}
//static inline TimeUnit getFreq() {
//  // Not true, just make something up to get good order of the numbers
//	return 1000000;
//}
#else
#include <sys/time.h>
typedef long int TimeUnit;
static inline TimeUnit getTime() {
  timeval tv;
	gettimeofday(&tv, 0);
	return (tv.tv_sec*1000000+tv.tv_usec);
}
static inline TimeUnit getFreq() {
	return 1000000;
}
//#else
//#include <time.h>
//typedef long int TimeUnit;
//static __inline TimeUnit getTime() {
//  timespec t;
//	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
//	return t.tv_sec*1000000000 + tv_nsec;
//}
//static inline TimeUnit getFreq() {
//	return 1000000000;
//}
#endif

//#elif defined (__INTEL_COMPILER)
//// INTEL COMPILER
//#pragma message("Using timer = Intel _rdtsc()")
//typedef unsigned long long TimeUnit;
//static __inline__ TimeUnit getTime() {
//	return _rdtsc();
//}
//static inline TimeUnit getFreq() {
//  // Not true, just make something up to get good order of the numbers
//	return 1000000;
//}
//
//// <CODE from="http://www.mcs.anl.gov/~kazutomo/rdtsc.html">
//#elif defined(__x86_64__)
//#warning("Using timer = rdtsc x86_64")
//
//typedef unsigned long long int TimeUnit;
//
//static __inline__ TimeUnit getTime()
//{
//  unsigned hi, lo;
//  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
//  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
//}
//static inline TimeUnit getFreq() {
//  // Not true, just make something up to get good order of the numbers
//	return 1000000;
//}
//
//#elif defined(__powerpc__)
//#warning("Using timer = rdtsc PowerPC")
//
//typedef unsigned long long int TimeUnit;
//
//static __inline__ TimeUnit getTime()
//{
//  unsigned long long int result=0;
//  unsigned long int upper, lower,tmp;
//  __asm__ volatile(
//                "0:                  \n"
//                "\tmftbu   %0           \n"
//                "\tmftb    %1           \n"
//                "\tmftbu   %2           \n"
//                "\tcmpw    %2,%0        \n"
//                "\tbne     0b         \n"
//                : "=r"(upper),"=r"(lower),"=r"(tmp)
//                );
//  result = upper;
//  result = result<<32;
//  result = result|lower;
//
//  return(result);
//}
//static inline TimeUnit getFreq() {
//  // Not true, just make something up to get good order of the numbers
//	return 1000000;
//}
//
//#else
//#include <time.h>
//static __inline__ TimeUnit getTime()
//{
//#warning("Using timer = clock()")
//	return clock();
//}
//static inline TimeUnit getFreq() {
//	return CLOCKS_PER_SEC;
//}
//#endif
//// </CODE>

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
