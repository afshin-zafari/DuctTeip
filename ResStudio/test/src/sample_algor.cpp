#include "sample_algor.hpp"

SampleAlgorithm::SampleAlgorithm()
{
    name.assign("chol");
    N = 12;
    n = 2;
    b = 3;
    M = new Data("A",N,N,this);
    if ( M->getParent())
        M->setDataHandle( M->getParent()->createDataHandle(M));
    M->setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
    M->setLocalNumBlocks(n,n);

    //LOG_INFO(LOG_DATA,"config.Nb=%d.\n",config.Nb);
    M->setPartition(b,b) ;
    addInOutData(M);

}
SampleAlgorithm::~SampleAlgorithm()
{
    delete M;
}
void SampleAlgorithm :: runKernels(IDuctteipTask *task){}
string SampleAlgorithm :: getTaskName(unsigned long) {}
void SampleAlgorithm :: taskFinished(IDuctteipTask *task,TimeUnit dur){}

