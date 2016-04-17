#ifndef __THREADS_HPP__
#define __THREADS_HPP__

// ===========================================================================
// Defines Threads interface, and contains platform specific threading code
// ===========================================================================

#ifdef __linux__
#include <stdlib.h>
#include <pthread.h>
#endif
#ifdef __sun
#include <unistd.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/types.h>
#include <sys/cpuvar.h>
#include <pthread.h>
#endif
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifndef _WIN32
#define PTHREADS
typedef pthread_t ThreadType;
typedef pthread_t ThreadIDType;
#else
typedef HANDLE ThreadType;
typedef DWORD ThreadIDType;
#endif

#undef LOG
#define LOG(a)
#include <iostream>
//#define LOG(a) { std::stringstream dbg; dbg << a; std::cerr << dbg.str(); }


// ===========================================================================
// ThreadUtil: Utility functions
// ===========================================================================
class ThreadUtil {
public:
	static void exit() {
#ifdef _WIN32
		ExitThread(0);
#else
		pthread_exit(0);
#endif
	}

  static void sleep(unsigned int millisec) {
#ifdef _WIN32
		Sleep(millisec);
#else
		usleep(millisec * 1000);
#endif
	}

	// hwcpuid is the os-specific cpu identifier
  static void setAffinity(size_t hwcpuid) {
#ifdef __sun
		if (processor_bind(P_LWPID, P_MYID, hwcpuid, NULL) != 0) {
			std::cerr << "processor_bind failed\n";
			::exit(1);
		}
#endif
#ifdef __linux__
			cpu_set_t affinityMask;
			CPU_ZERO(&affinityMask);
			CPU_SET(hwcpuid, &affinityMask);
			if (sched_setaffinity(0, sizeof(cpu_set_t), &affinityMask) !=0) {
				std::cerr << "sched_setaffinity failed." << std::endl;
				::exit(1);
			}

			//		cpu_set_t cpuset;
			//		CPU_ZERO(&cpuset);
			//		CPU_SET(hwcpuid, &cpuset);
			//		pthread_attr_setaffinity_np(&attr, CPU_SETSIZE, &cpuset);

		//		const size_t numCPUs = getNumCPUs();
		//		cpu_set_t *cpuset = CPU_ALLOC(numCPUs);
		//		assert(cpuset != NULL);
		//		const size_t cpuSetSize = CPU_ALLOC_SIZE(numCPUs);
		//		CPU_ZERO_S(cpuSetSize, cpuset);
		//		CPU_SET_S(hwcpuid, cpuSetSize, cpuset);
    //    // pthread_setaffinity_np:
    //    //  "If the call is successful, and the thread is not currently running 
    //    //  on one of the CPUs in cpuset, then it is migrated to one of those CPUs."
		//		if (pthread_setaffinity_np(pthread_self(), cpuSetSize, cpuset) != 0) {
		//			CPU_FREE(cpuset);
		//			std::cerr << "sched_setaffinity failed." << std::endl;
		//			::exit(1);
		//		}
		//		CPU_FREE(cpuset);
#endif
#ifdef _WIN32
  SetThreadAffinityMask(GetCurrentThread(), 1<<hwcpuid);
  // The thread should be rescheduled immediately:
  // "If the new thread affinity mask does not specify the processor that is
  //  currently running the thread, the thread is rescheduled on one of the
  //  allowable processors."
#endif
	}

  static int getNumCPUs() {
#ifdef __sun
		int numCPUs = (processorid_t) sysconf(_SC_NPROCESSORS_ONLN);
		int online = 0;
		for (int i = 0; i < numCPUs; ++i)
			if (p_online(i, P_STATUS) == P_ONLINE)
				++online;
		return online;
#endif
#ifdef __linux__
		return sysconf(_SC_NPROCESSORS_ONLN);
#endif
#ifdef _WIN32
		SYSTEM_INFO m_si = {0};
		GetSystemInfo(&m_si);
		return (int) m_si.dwNumberOfProcessors;
#endif
	}

	static ThreadIDType getCurrentThreadId() {
#ifdef _WIN32
		return GetCurrentThreadId();
#else
		return pthread_self();
#endif
	}
};

#ifndef _WIN32
extern "C" void *spawnFreeThread(void *arg);
extern "C" void *spawnThread(void *arg);
#endif

// ===========================================================================
// FreeThread: Thread that is not locked to a certain core
// ===========================================================================
class FreeThread {
private:
	ThreadType thread;
	void *data;
	FreeThread(const FreeThread &);
	const FreeThread &operator=(const FreeThread &);

#ifdef _WIN32
	static DWORD WINAPI spawnFreeThread(LPVOID arg) {
		((FreeThread *) arg)->run();
    ThreadUtil::exit();
    return 0;
	}
#endif

public:
	FreeThread() {}
	virtual void run() = 0;

	void start(void *data_ = 0) {
		this->data = data_;
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
	void join() {
#ifdef _WIN32
		WaitForSingleObject(thread, INFINITE);
#else
		void *status;
		pthread_join(thread, &status);
#endif
	}
};

// ===========================================================================
// Thread: Thread locked to core #hwcpuid
// ===========================================================================
class Thread {
private:
	ThreadType thread;
#ifdef _WIN32
  ThreadIDType threadID;
#endif

	Thread(const Thread &);
	const Thread &operator=(const Thread &);

#ifdef _WIN32
	static DWORD WINAPI spawnThread(LPVOID arg) {
		((Thread *) arg)->run();
    ThreadUtil::exit();
    return 0;
	}
#endif

protected:
	void *data;

public:
	int cpuid;

	virtual void run() = 0;

public:
	Thread() {}
	// About destructor:
	// Perhaps joinable threads should automatically join when leaving 
	// scope? Otherwise its easy to destruct threads while they are 
	// running.

	// If destructor is added to Thread that joins, any subclass will
	// still be destructed before the join() is reached. Mix-Ins?

	// hwcpuid is the os-specific cpu identifier
	void start(size_t hwcpuid, void *data_ = 0) {
		this->data = data_;
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

#ifdef __linux__
		/*
			const size_t numCPUs = getNumCPUs();
			cpu_set_t *cpuset = CPU_ALLOC(numCPUs);
			assert(cpuset != NULL);
			const size_t cpuSetSize = CPU_ALLOC_SIZE(numCPUs);
			CPU_ZERO_S(cpuSetSize, cpuset);
			CPU_SET_S(id, cpuSetSize, cpuset);
			pthread_attr_setaffinity_np(&attr, cpuSetSize, cpuset);
			CPU_FREE(cpuset);

		  // cpu_set_t cpuset;
		  // CPU_ZERO(&cpuset);
		  // CPU_SET(hwcpuid, &cpuset);
		  // pthread_attr_setaffinity_np(&attr, CPU_SETSIZE, &cpuset);

		*/

#endif
		cpuid = hwcpuid;

		if (pthread_create(&thread, &attr, spawnThread, (void *) this) != 0) {
			std::cerr << "pthread_create failed" << std::endl;
			::exit(1);
		}
		pthread_attr_destroy(&attr);
#endif
	}

	void join() {
#ifdef _WIN32
		WaitForSingleObject(thread, INFINITE);
#else
		void *status;
		pthread_join(thread, &status);
#endif
	}

	// [DEBUG] returns operating system thread identifier
  int getThreadId() {
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
#endif

#endif // __THREADS_HPP__

