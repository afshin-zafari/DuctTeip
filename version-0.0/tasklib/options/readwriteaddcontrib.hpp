#ifndef __READWRITEADDCONTRIB_HPP_
#define __READWRITEADDCONTRIB_HPP_

// ============================================================================
// ReadWriteAddContrib
// ============================================================================
// Class defining access types.
// All selection on access type must go through this class.
class ReadWriteAddContrib {
public:
    // RULES:
    //   needsLock = modifiesData && commutesWithSomeOtherAccessType
    //   !modifiesData => commutes with self

    // This enum is used in interfaces, and since enums cannot
    // be forward declared, it is treated as an int instead of
    // its enum type to avoid including this class in the interface.
    // ?? wouldn't the appearance of a type in the interface imply
    //    that the callee needs to know about the types anyhow?
    enum Type { read = 0, write, add, addcontrib, numAccesses };

    // static enum -> type
    template<int n>
    struct AccessTag { typedef void is_type; };
    typedef AccessTag<read>  read_t;
    typedef AccessTag<write> write_t;
    typedef AccessTag<add>   add_t;
    typedef AccessTag<addcontrib>   addcontrib_t;

    // defaults
    template<class T>          inline static bool needsLock(T)    { return false; }
    template<class A, class B> inline static bool commutes(A, B)  { return false; }
    template<class T>          inline static bool modifiesData(T) { return true; }

    // settings
    inline static bool needsLock(add_t) { return true; }
    inline static bool commutes(read_t, read_t) { return true; }
    inline static bool commutes(add_t, add_t) { return true; }
    inline static bool commutes(add_t, addcontrib_t) { return true; }
    inline static bool commutes(addcontrib_t, add_t) { return true; }
    inline static bool commutes(addcontrib_t, addcontrib_t) { return true; }
    inline static bool modifiesData(read_t) { return false; }
//    inline static bool modifiesData(addcontrib_t) { return false; }
};

#endif // __READWRITEADDCONTRIB_HPP_
