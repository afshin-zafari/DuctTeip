#ifndef __READWRITEADD_HPP__
#define __READWRITEADD_HPP__

// ============================================================================
// ReadWriteAdd
// ============================================================================
// Class defining access types.
// All selection on access type must go through this class.
class ReadWriteAdd {
public:

    enum Type { read = 0, write, add, numAccesses };

    // static enum -> type
    template<int n>
    struct AccessTag { typedef void is_type; };
    typedef AccessTag<read>  read_t;
    typedef AccessTag<write> write_t;
    typedef AccessTag<add>   add_t;

    // defaults
    template<class A, class B> static bool commutes(A, B)  { return false; }
    template<class T>          static bool modifiesData(T) { return true; }
    template<class T>          static bool readsData(T)    { return true; }

    // settings
    static bool commutes(read_t, read_t) { return true; }
    static bool commutes(add_t, add_t) { return true; }
    static bool modifiesData(read_t) { return false; }
};

#endif // __READWRITEADD_HPP__
