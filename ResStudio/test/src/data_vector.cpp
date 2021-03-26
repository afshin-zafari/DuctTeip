#include "data_vector.hpp"

DataVector::DataVector(int m, Context *context)
{
    vecData = new std::vector<Data*> (m);
    int b = 1;
    int n = 1;
    for( int i=0; i<  vecData->size(); i++ )
    {
        Data *d = new Data("d",1,1,context);
        d->setDataHandle( d->getParent()->createDataHandle(d));
        d->setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
        d->setLocalNumBlocks(n,n);

        d->setPartition(b,b) ;
        context->addInOutData(d);
        vecData->at(i) = d;
    }

}