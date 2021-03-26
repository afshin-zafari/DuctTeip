#include "ductteip.hpp"

class SampleAlgorithm : public Algorithm
{
    private: 
        Data *M;
        int N,n,b;
    public:
    SampleAlgorithm();    
    ~SampleAlgorithm();
    void runKernels(IDuctteipTask *task);
    string getTaskName(unsigned long) ;
    void taskFinished(IDuctteipTask *task,TimeUnit dur);

};