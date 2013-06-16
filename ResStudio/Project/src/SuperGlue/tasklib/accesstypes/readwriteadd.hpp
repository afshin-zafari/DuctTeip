#ifndef __READWRITEADD_HPP__
#define __READWRITEADD_HPP__

// ============================================================================
// ReadWriteAdd
// ============================================================================
// Class defining access types.
// All selection on access type must go through this class.
class ReadWriteAdd {
public:
	// RULES:
	//   needsLock = modifiesData && commutesWithSomeOtherAccessType
	//   !modifiesData => commutes with self

	// This enum is used in interfaces, and since enums cannot
	// be forward declared, it is treated as an int instead of
	// its enum type to avoid including this class in the interface.
	// ?? wouldn't the appearance of a type in the interface imply
	//    that the callee needs to know about the types anyhow?
	enum Type { read = 0, write, add, numAccesses };

	template<class T> struct static_constraint {};

	// static enum -> type
	template<int n>
	struct AccessTag { typedef void is_type; };
	typedef AccessTag<read> read_t;
	typedef AccessTag<write> write_t;
	typedef AccessTag<add> add_t;

	// static needsLock
	template<class T>
  inline static bool needsLock(T) { 
		static_constraint<typename T::is_type>(); 
		return false;
	}
  inline static bool needsLock(add_t) { return true; }

	// static commutes(type, type)
	template<class A, class B>
	inline static bool commutes(A, B) {
		static_constraint<typename A::is_type>();
		static_constraint<typename B::is_type>();
		return false;
	}
	inline static bool commutes(read_t, read_t) { return true; }
	inline static bool commutes(add_t, add_t) { return true; }

	// static modifiesData
	template<class T>
  inline static bool modifiesData(T) { 
		static_constraint<typename T::is_type>(); 
		return true;
	}
  inline static bool modifiesData(read_t) { return false; }
};

#endif // __READWRITEADD_HPP__
