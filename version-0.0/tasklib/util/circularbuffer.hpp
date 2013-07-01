#ifndef __CIRCULARBUFFER_HPP__
#define __CIRCULARBUFFER_HPP__

#include <vector>
#include "threads/mutex.hpp"

template<class T>
class CircularBuffer {
private:
  Mutex insertLock;
  Mutex stealLock;

  CircularBuffer(const CircularBuffer &);
  const CircularBuffer &operator=(const CircularBuffer &);


  void resize() {
    std::vector<T> newBuffer(buffer.size() * 2);
    if (start < end) {
      std::copy(buffer.begin() + start, buffer.begin() + end, newBuffer.begin());
      start = 0;
      end = end - start;
      std::swap(newBuffer, buffer);
    }
    else {
      size_t n = buffer.size() - start;
      std::copy(buffer.begin() + start, buffer.end(), newBuffer.begin());
      std::copy(buffer.begin(), buffer.begin() + end, newBuffer.begin() + n);
      start = 0;
      end = end + n;
      std::swap(newBuffer, buffer);
    }
  }

public:
  std::vector<T> buffer;
  volatile size_t start;
  volatile size_t end;

  CircularBuffer() : buffer(512), start(0), end(0) {}
  CircularBuffer(size_t size) : buffer(size), start(0), end(0) {}

  inline bool empty() const { return start == end; }

  inline void push_back(T elem) {
    MutexLock lock(insertLock);
    size_t newEnd = end + 1;
    if (newEnd == buffer.size())
      newEnd = 0;

    if (start == newEnd) {
      resize();
      // here we are guaranteed that end does not need wrapping
      buffer[end++] = elem;
    }
    else {
      buffer[end] = elem;
      end = newEnd;
    }
  }

  inline void push_front(T elem) {
    MutexLock lockInsert(insertLock); // to prevent a concurrent push_back() to take the last place
    MutexLock lockSteal(stealLock);
    size_t newStart;
    if (start == 0)
      newStart = buffer.size() - 1;
    else
      newStart = start - 1;

    if (newStart == end) {
      resize();
      // not very nice layout
      start = buffer.size() - 1;
      buffer[start] = elem;
    }
    else {
      buffer[newStart] = elem;
      start = newStart;
    }
  }

  inline bool get(T &elem) {
    if (empty())
      return false;
    MutexLock lock(stealLock);
    if (empty())
      return false;
    elem = buffer[start++];
    if (start == buffer.size())
      start = 0;
    return true;
  }

  inline bool steal(T &elem) {
    if (empty())
      return false;
    MutexLock lockInsert(insertLock);
    MutexLock lockSteal(stealLock);
    if (empty())
      return false;
    if (end == 0)
      end = buffer.size() - 1;
    else
      --end;
    elem = buffer[end];
    return true;
  }
};


#endif // __CIRCULARBUFFER_HPP__
