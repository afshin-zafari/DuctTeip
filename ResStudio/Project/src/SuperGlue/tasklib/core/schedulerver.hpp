#ifndef __SCHEDULERVER_HPP__
#define __SCHEDULERVER_HPP__

#include <cstring> // memset
#include <algorithm> // max_elements
#include "options/readwriteadd.hpp"

namespace detail {
    template<typename Options, typename T>
    struct VersionStorage {
        enum { storageSize = Options::AccessInfoType::numAccesses };
    };

    template<typename Options>
    struct VersionStorage<Options, ReadWriteAdd> {
        enum { storageSize = 2 };
    };
}

// ============================================================================
// SchedulerVersion
// ============================================================================
template<typename Options>
class SchedulerVersion
 : public detail::VersionStorage<Options, typename Options::AccessInfoType> {
public:
    typedef typename Options::AccessInfoType AccessInfo;
    typedef typename Options::version_t version_t;
    typedef detail::VersionStorage<Options, typename Options::AccessInfoType> VersionStorage;
private:

    version_t requiredVersion[VersionStorage::storageSize];

    // increase scheduler version on static type
    template<int n, int type>
    struct Increase {
        static void increase(version_t *requiredVersion, version_t nextVer) {
            Increase<n-1,type>::increase(requiredVersion, nextVer);
            if (!AccessInfo::commutes(typename AccessInfo::template AccessTag<type>(),
                                      typename AccessInfo::template AccessTag<n-1>()))
                requiredVersion[n-1] = nextVer;
        }
    };
    template<int type>
    struct Increase<0, type> {
        static void increase(version_t * /*requiredVersion*/, version_t /*nextVer*/) {}
    };

    // increase scheduler version on dynamic type
    template<int n, bool stop = (n==0)>
    struct IncreaseAux {
        static void increase(int type, version_t *requiredVersion, version_t nextVer) {
            if (type == n-1)
                Increase<AccessInfo::numAccesses, n-1>::increase(requiredVersion, nextVer);
            else
                IncreaseAux<n-1>::increase(type, requiredVersion, nextVer);
        }
    };
    template<int n>
    struct IncreaseAux<n, true> {
        static void increase(int /*type*/, version_t * /*requiredVersion*/, version_t /*nextVer*/) {}
    };

    template<typename TYPE>
    version_t scheduleImpl(int type, TYPE) {
        version_t ver = requiredVersion[type];
        IncreaseAux<AccessInfo::numAccesses>::increase(type, requiredVersion, nextVersion());
        return ver;
    }

    version_t scheduleImpl(int type, ReadWriteAdd) {
        version_t nextVersion = std::max(requiredVersion[0], requiredVersion[1]);
        switch (type) {
        case ReadWriteAdd::read:
            requiredVersion[1] = nextVersion+1;
            return requiredVersion[0];
        case ReadWriteAdd::add:
            requiredVersion[0] = nextVersion+1;
            return requiredVersion[1];
        }
        requiredVersion[0] = requiredVersion[1] = nextVersion+1;
        return nextVersion;
    }

public:
    SchedulerVersion() {
        std::memset(requiredVersion, 0, sizeof(requiredVersion));
    }

    version_t schedule(int type) { return scheduleImpl(type, AccessInfo()); }

    version_t nextVersion() {
        return *std::max_element(requiredVersion, requiredVersion + VersionStorage::storageSize)+1;
    }
};

#endif // __SCHEDULERVER_HPP__
