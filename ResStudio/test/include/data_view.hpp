#include "ductteip.hpp"
#include <vector>
#include <list>
class DataView
{
public:
    std::vector<Data*> * origin;
    int start, end;
    void register_read();
};
class DataViewContext: public Algorithm
{
    public:
    DataViewContext();
    void test_01();
    void test_02();
    void  runKernels(IDuctteipTask *task){}
    string getTaskName(unsigned long) {}
    void  taskFinished(IDuctteipTask *task,TimeUnit dur){}
};