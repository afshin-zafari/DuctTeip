#ifndef __IHANDLE_HPP__
#define __IHANDLE_HPP__

#include "handlelistener.hpp"
#include <cstddef> // size_t

// ============================================================================
// Option: Lockable
// ============================================================================
template<typename Options, bool Lockable> struct IHandle_Lockable {};
template<typename Options> struct IHandle_Lockable<Options, true> {
  // low level API for getting several locks and release if fail
  virtual bool getLock() = 0;

  virtual bool getLockOrNotify(LockListener<Options> *listener) = 0;

	virtual void releaseLock() = 0;
};

// ============================================================================
// Interface IHandle
// ============================================================================
template<typename Options>
class IHandle
	: public IHandle_Lockable<Options, Options::Lockable>
{
public:

  IHandle() {}
  virtual ~IHandle() {};

  virtual void increaseCurrentVersion() = 0;

  // scheduler API {
  virtual std::size_t schedulerGetRequiredVersion(int type) = 0;
  virtual void schedulerIncrease(int type) = 0;
  // }

  // running API {
  // get current version [[### TODO: Who uses this? Debug? ]]
  virtual std::size_t getCurrentVersion() = 0;
  // }

  virtual bool isVersionAvailableOrNotify(VersionListener<Options> *listener, 
																					std::size_t version) = 0;
};

#endif // __IDATAHANDLE_HPP__
