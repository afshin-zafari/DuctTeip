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
    std::vector<DataView> * partition ( std::vector<Data*> * data_vector, int n );
    void  runKernels(IDuctteipTask *task){}
    string getTaskName(unsigned long) {}
    void  taskFinished(IDuctteipTask *task,TimeUnit dur){}
};