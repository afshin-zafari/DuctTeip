#include "ductteip.hpp"
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