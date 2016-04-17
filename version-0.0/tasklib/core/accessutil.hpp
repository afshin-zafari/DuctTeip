#ifndef __ACCESSUTIL_HPP__
#define __ACCESSUTIL_HPP__

template<class Options>
class AccessUtil {
    typedef typename Options::AccessInfoType AccessInfo;

    template<int n, typename Predicate, bool stop = (n==0)>
    struct Aux {
        static bool check(int type) {
            if (type == n-1)
                return Predicate::check(typename AccessInfo::template AccessTag<n-1>());
            else
                return Aux<n-1, Predicate>::check(type);
        }
    };
    template<int n, typename Predicate>
    struct Aux<n, Predicate, true> {
        // should never be called, but required for compilation
        static bool check(int) { return false; }
    };

    template<int n, typename AccessType, bool stop = (n==0)>
    struct Commutes {
        static bool check() {
            if (AccessInfo::commutes( AccessType(),
                                      typename AccessInfo::template AccessTag<n-1>() ))
                return true;
            return Commutes<n-1, AccessType>::check();
        }
    };
    template<int n, typename AccessType>
    struct Commutes<n, AccessType, true> {
        static bool check() { return false; }
    };

    struct NeedsLockPredicate {
        template<typename T>
        static bool check(T) {
            return Commutes<AccessInfo::numAccesses, T>::check();
        }
    };

    struct ReadsDataPredicate {
        template<typename T>
        static bool check(T) { return AccessInfo::readsData(T()); }
    };

    struct ModifiesDataPredicate {
        template<typename T>
        static bool check(T) { return AccessInfo::modifiesData(T()); }
    };

public:
    static bool needsLock(int type) {
        return modifiesData(type) &&
            Aux<AccessInfo::numAccesses, NeedsLockPredicate >::check(type);
    }

    static bool readsData(int type) {
        return Aux<AccessInfo::numAccesses, ReadsDataPredicate>::check(type);
    }

    static bool modifiesData(int type) {
        return Aux<AccessInfo::numAccesses, ModifiesDataPredicate>::check(type);
    }
};

#endif // __ACCESSUTIL_HPP__
