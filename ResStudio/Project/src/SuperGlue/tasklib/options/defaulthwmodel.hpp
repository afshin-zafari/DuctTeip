#ifndef __DEFAULTHWMODEL_HPP_
#define __DEFAULTHWMODEL_HPP_

struct DefaultHardwareModel {
    enum { CACHE_LINE_SIZE = 64 };
    static int cpumap(int id) { return id; }
};

#endif // __DEFAULTHWMODEL_HPP_
