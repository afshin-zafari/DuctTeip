#ifndef __HANDLELISTENER_HPP__
#define __HANDLELISTENER_HPP__

template<typename Options>
struct VersionListener {
	typedef typename Options::IHandleType IHandle;
  virtual void notifyVersion(IHandle *handle) = 0;
};

template<typename Options>
struct LockListener {
	typedef typename Options::IHandleType IHandle;
  virtual void notifyLock(IHandle *handle) = 0;
};

#endif // __HANDLELISTENER_HPP__
