#ifndef __SCHEDULERVER_HPP__
#define __SCHEDULERVER_HPP__

// ============================================================================
// SchedulerVersion
// ============================================================================
template<typename Options>
class SchedulerVersion {
public:
  typedef typename Options::AccessInfoType AccessInfo;
private:
  size_t requiredVersion[AccessInfo::numAccesses];

	// increase scheduler version on static type
	template<int n, int type>
	struct Increase { 
		static inline void increase(size_t *requiredVersion, size_t nextVer) {
			Increase<n-1,type>::increase(requiredVersion, nextVer);
			if (!AccessInfo::commutes(typename AccessInfo::template AccessTag<type>(),
																typename AccessInfo::template AccessTag<n-1>()))
				requiredVersion[n-1] = nextVer;
		}
	};
	template<int type>
	struct Increase<0, type> { 
		static inline void increase(size_t */*requiredVersion*/, size_t /*nextVer*/) {}
	};

	// increase scheduler version on dynamic type
	template<int n, bool stop = (n==0)>
	struct IncreaseAux {
		static inline void increase(int type, size_t *requiredVersion, size_t nextVer) {
			if (type == n-1)
				Increase<AccessInfo::numAccesses, n-1>::increase(requiredVersion, nextVer);
			else
				IncreaseAux<n-1>::increase(type, requiredVersion, nextVer);
		}
	};
	template<int n>
	struct IncreaseAux<n, true> {
		static inline void increase(int /*type*/, size_t * /*requiredVersion*/, size_t /*nextVer*/) {}
	};

public:
  SchedulerVersion() { 
    std::memset(requiredVersion, 0, sizeof(requiredVersion)); 
  }

  inline size_t get(int type) { return requiredVersion[type]; }

	inline void increase(int type) {
		size_t nextVer = *std::max_element(requiredVersion, requiredVersion+AccessInfo::numAccesses)+1;
		IncreaseAux<AccessInfo::numAccesses>::increase(type, requiredVersion, nextVer);
	}
};

#endif // __SCHEDULERVER_HPP__
