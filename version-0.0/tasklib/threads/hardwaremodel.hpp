#ifndef __HARDWAREMODEL_HPP__
#define __HARDWAREMODEL_HPP__
 
// class responsible for detecting the hardware topology
// currently only a mockup

class HardwareModel {
private:
public:

	// return hardware ID of all threads in an order such
	// that any two cores sharing something are adjacent.
	const inline int *getHWID() {
		static int hwid[] = {0, 1, 2, 3, 4, 5, 6, 7};
		return hwid;
	}

	void getL1(int n, int **begin, int **end) {
		static int hwid[] = {0, 1, 2, 3, 4, 5, 6, 7};
		static int l1[] = {0, 0, 1, 1, 2, 2, 3, 3};

		unsigned int i;
		for (i = 0; i < sizeof(l1)/sizeof(int); ++i)
			if (l1[i] == n) {
				*begin = &hwid[i];
				break;
			}

		for (; i < sizeof(l1)/sizeof(int); ++i)
			if (l1[i] != n)
				break;

		*end = &hwid[i];
	}

	enum { CACHE_LINE_SIZE = 64 };
	//	static inline int getCacheLineSize() { return 64; }
};

#endif // __HARDWAREMODEL_HPP__
