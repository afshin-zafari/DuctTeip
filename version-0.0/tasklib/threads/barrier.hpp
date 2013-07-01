#ifndef __BARRIER_HPP__
#define __BARRIER_HPP__

// 1) One thread calls barrier() to initiate a barrier
// 2) Threads see their waitAtBarrier flag and calls waitAtBarrier()
// 2a) Threads take a lock: 
// 2b) Threads can wait for new work on this lock,
// 2c) or wait at a barrier on this lock

// Threads take a lock: 
//   SignalLock lock(b.getLock())
// This lock must be hold to check and get tasks
// If no tasks exists, a thread should either wait at a barrier
// or wait for new tasks, using the lock.

// It is probably possible to create a datatype which corresponds
// to a SignalLock stateLock(stateSignal), and require an argument
// of this datatype in all methods that require the lock to be held.
// However, its possible to release the lock, and then the system
// will leak anyhow.

#include "util/atomic.hpp"
#include "threads/mutex.hpp"

#include <string>
#include <iostream>
#include <sstream>
//#undef DBG
//#define DBG(a) { std::stringstream dbg; dbg << pthread_self() << ": " << a; std::cerr << dbg.str(); }

struct ThreadBarrierData {
	volatile bool waitAtBarrier;
	ThreadBarrierData() : waitAtBarrier(false) {}
};

// struct BarrierDataCarrier {
//   ThreadBarrierData barrierData;
// }

class BarrierProtocol {
private:
	volatile size_t barrierCounter;
	volatile bool barrierAborted;
  volatile int state;
	const size_t numThreads;
	Signal counterSignal;
	Signal stateSignal;

	// thread must have seen waitAtBarrier before calling
	void waitAtBarrier(SignalLock &lock) {
		++barrierCounter;
		int mystate = state;
		//		DBG("Barrier Counter = " << barrierCounter 
		//				<< " numThreads = " << numThreads << std::endl);
		if (barrierCounter == numThreads) {
			// done!
			SignalLock stateLock(stateSignal);
			state = 1-state;
			//			DBG("Last into barrier! New state " << state << std::endl);
			lock.broadcast();
			stateLock.signal();
		}
		else {
			//			DBG("Wait at barrier " << barrierCounter << std::endl);
			while (state == mystate && !barrierAborted)
				lock.wait();
			//			DBG("Barrier done " << barrierCounter << std::endl);
		}
	}

public:

	BarrierProtocol(size_t numThreads_)
		: barrierCounter(0), barrierAborted(false),
			state(0), numThreads(numThreads_) {}

	// Called from WorkerThread: Wait at barrier if requested.
	bool waitAtBarrier(ThreadBarrierData &data, SignalLock &lock) {
		if (!data.waitAtBarrier)
			return false;
		data.waitAtBarrier = false;
		waitAtBarrier(lock);
    return true;
	}

	// cannot be invoked by more than one thread at a time
	template<class BarrierDataCarrier>
	void barrier(BarrierDataCarrier **threads) {
		// to prevent several threads from calling barrier() concurrently!
		static Mutex barrierMutex;
		MutexLock barrierLock(barrierMutex);

		//DBG("BARRIER-MAIN: " << before << "\n");
		//		DBG("Barrier\n");
		do {
			//			DBG("Barrier loop\n");
			//		if (barrierAborted)
			//			DBG("BARRIER-ABORTED: REDONING");
			SignalLock lock(counterSignal);
			barrierCounter = 0;
			barrierAborted = false;
			Atomic::memory_fence(); // probably not needed

			SignalLock stateLock(stateSignal);

			int mystate = state;
			
			for (size_t i = 0; i < numThreads; ++i) {
				assert(!threads[i]->getBarrierData().waitAtBarrier);
				//DBG("thread " << i << " wait" << std::endl);
				threads[i]->getBarrierData().waitAtBarrier = true;
			}
			Atomic::memory_fence();
			lock.broadcast();

			//DBG("before wait state: " << (after - before) / (getFreq() / 1000.0) << " ms" << endl);

			lock.release();
			//			DBG("Waiting for threads\n");
			while (state == mystate)
				stateLock.wait();

			//DBG("after wait state: " << (after - before) / (getFreq() / 1000.0) << " ms" << endl);
			Atomic::memory_fence();
			//			DBG("Abort status: " << barrierAborted << "\n");
		} while (barrierAborted);
		//		DBG("Barrier finished\n");
	}

	inline Signal &getLock() {
		return counterSignal;
	}

	// abort barrier
	inline void signalNewWork() {
		SignalLock lock(counterSignal);
		barrierAborted = true;
		lock.broadcast();
	}

	// [DEBUG]
	inline int getState() { return state; }

};

class BarrierProtocolThread {
private:
	ThreadBarrierData barrierData;
	BarrierProtocol &barrierProtocol;

public:
	BarrierProtocolThread(BarrierProtocol &barrierProtocol_)
		: barrierProtocol(barrierProtocol_) {}
	inline bool waitAtBarrier(SignalLock &lock) { 
		return barrierProtocol.waitAtBarrier(barrierData, lock); 
	}
	inline ThreadBarrierData &getBarrierData() { 
		return barrierData;
	}
	inline Signal &getBarrierLock() {
		return barrierProtocol.getLock();
	}

};

#endif // __BARRIER_HPP__
