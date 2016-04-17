#ifndef __THREADS_HPP__
#define __THREADS_HPP__

// ===========================================================================
// Defines Threads interface, and contains platform specific threading code
// ===========================================================================

#ifdef __linux__
#include <stdlib.h>
#include <cstdlib>
#include <pthread.h>
#endif
#ifdef __sun
#include <unistd.h>
#include <cstdlib>
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/types.h>
#include <sys/cpuvar.h>
#include <pthread.h>
#endif
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <iostream>
#endif

#ifndef _WIN32
#define PTHREADS
typedef pthread_t ThreadType;
typedef pthread_t ThreadIDType;
#else
typedef HANDLE ThreadType;
typedef DWORD ThreadIDType;
#endif


namespace detail {

// ===========================================================================
// ThreadImplAux: pthread implementations depending on platform
// ===========================================================================
#ifdef __sun
struct ThreadImplAux {
    static void setAffinity(size_t hwcpuid) {
        if (processor_bind(P_LWPID, P_MYID, hwcpuid, NULL) != 0) {
            std::cerr << "processor_bind failed\n";
            ::exit(1);
        }
    }

    static int getNumCPUs() {
        int numCPUs = (processorid_t) sysconf(_SC_NPROCESSORS_ONLN);
        int online = 0;
        for (int i = 0; i < numCPUs; ++i)
            if (p_online(i, P_STATUS) == P_ONLINE)
                ++online;
        return online;
    }
};
#elif __linux__
struct ThreadImplAux {
    static void setAffinity(size_t hwcpuid) {
        cpu_set_t affinityMask;
        CPU_ZERO(&affinityMask);
        CPU_SET(hwcpuid, &affinityMask);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &affinityMask) !=0) {
            std::cerr << "sched_setaffinity failed." << std::endl;
            ::exit(1);
        }
    }
    static int getNumCPUs() { return sysconf(_SC_NPROCESSORS_ONLN); }
};
#endif

// ===========================================================================
// ThreadImpl: Implementations depending on platform
// ===========================================================================
#ifdef PTHREADS
  struct ThreadImpl {
      static void exit() { pthread_exit(0); }
      static void sleep(unsigned int millisec) { usleep(millisec * 1000); }
      static void setAffinity(size_t hwcpuid) { ThreadImplAux::setAffinity(hwcpuid); }
      static int getNumCPUs() { return ThreadImplAux::getNumCPUs(); }
      static ThreadIDType getCurrentThreadId() { return pthread_self(); }
      static void join(ThreadType thread) {
          void *status;
          pthread_join(thread, &status);
      }
  };

#elif _WIN32
  struct ThreadImpl {
      static void exit() { ExitThread(0); }
      static void sleep(unsigned int millisec) { Sleep(millisec); }
      static void setAffinity(size_t hwcpuid) {
          SetThreadAffinityMask(GetCurrentThread(), 1<<hwcpuid);
          // The thread should be rescheduled immediately:
          // "If the new thread affinity mask does not specify the processor that is
          //  currently running the thread, the thread is rescheduled on one of the
          //  allowable processors."
      }
      static int getNumCPUs() {
          SYSTEM_INFO m_si = {0};
          GetSystemInfo(&m_si);
          return (int) m_si.dwNumberOfProcessors;
      }
      static ThreadIDType getCurrentThreadId() { return GetCurrentThreadId(); }
      static void join(ThreadType thread) { WaitForSingleObject(thread, INFINITE); }

  };
#else
#error Not implemented for this platform
#endif

} // namespace detail

// ===========================================================================
// ThreadUtil: Utility functions
// ===========================================================================
class ThreadUtil {
public:
    static void exit() { detail::ThreadImpl::exit(); }
    static void sleep(unsigned int millisec) { detail::ThreadImpl::sleep(millisec); }
    static int getNumCPUs() { return detail::ThreadImpl::getNumCPUs(); }
    static ThreadIDType getCurrentThreadId() { return detail::ThreadImpl::getCurrentThreadId(); }
    // hwcpuid is the os-specific cpu identifier
    static void setAffinity(size_t hwcpuid) {
        hwcpuid = hwcpuid % getNumCPUs();
        detail::ThreadImpl::setAffinity(hwcpuid); 
    }
};

#ifndef _WIN32
extern "C" void *spawnFreeThread(void *arg);
extern "C" void *spawnThread(void *arg);
#else
DWORD WINAPI spawnFreeThread(LPVOID arg);
DWORD WINAPI spawnThread(LPVOID arg);
#endif

// ===========================================================================
// FreeThread: Thread that is not locked to a certain core
// ===========================================================================
class FreeThread {
private:
    FreeThread(const FreeThread &);
    const FreeThread &operator=(const FreeThread &);

protected:
    ThreadType thread;

public:
    FreeThread() {}
    // About destructor:
    // Perhaps joinable threads should automatically join when leaving
    // scope? Otherwise its easy to destruct threads while they are
    // running.

    // But any subclass will still be destructed before the join() is reached. Mix-Ins?

    virtual void run() = 0;

    void start() {
#ifdef _WIN32
        DWORD threadID;
        thread = CreateThread(NULL, 0, &spawnFreeThread, (void *) this, 0, &threadID);
#else
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        if (pthread_create(&thread, &attr, spawnFreeThread, (void *) this) != 0) {
            std::cerr << "pthread_create failed" << std::endl;
            ::exit(1);
        }
        pthread_attr_destroy(&attr);
#endif
    }
    void join() { detail::ThreadImpl::join(thread); }
};

// ===========================================================================
// Thread: Thread locked to core #hwcpuid
// ===========================================================================
class Thread : public FreeThread {
private:
#ifdef _WIN32
    ThreadIDType threadID;
#endif

public:
#ifndef _WIN32
    int cpuid;
#endif

    Thread() {}

    // hwcpuid is the os-specific cpu identifier
    void start(size_t hwcpuid) {
        // On linux: set affinity as property before creating task
        // On solaris: set affinity in spawnThread when task is just started
        // On windows: also when task is created. see below.
#ifdef _WIN32
        thread = CreateThread(NULL, 0, &spawnThread, (void *) this, 0, &threadID);
        SetThreadAffinityMask(thread, 1<<hwcpuid);
#else
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        cpuid = hwcpuid;

        if (pthread_create(&thread, &attr, spawnThread, (void *) this) != 0) {
            std::cerr << "pthread_create failed" << std::endl;
            ::exit(1);
        }
        pthread_attr_destroy(&attr);
#endif
    }

    // [DEBUG] returns operating system thread identifier
    ThreadIDType getThreadId() {
#ifdef _WIN32
        return threadID;
#else
        return thread;
#endif
    }
};

#ifndef _WIN32
extern "C" void *spawnFreeThread(void *arg) {
    ((FreeThread *) arg)->run();
    ThreadUtil::exit();
    return 0;
}
extern "C" void *spawnThread(void *arg) {
    Thread *thisThread = (Thread *) arg;

    ThreadUtil::setAffinity(thisThread->cpuid);

    thisThread->run();
    ThreadUtil::exit();
    return 0;
}
#elif _WIN32
    DWORD WINAPI spawnFreeThread(LPVOID arg) {
        ((FreeThread *) arg)->run();
        ThreadUtil::exit();
        return 0;
    }
    DWORD WINAPI spawnThread(LPVOID arg) {
        ((Thread *) arg)->run();
        ThreadUtil::exit();
        return 0;
    }
#endif

#endif // __THREADS_HPP__

