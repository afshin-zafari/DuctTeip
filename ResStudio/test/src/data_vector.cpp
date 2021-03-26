#include "data_vector.hpp"
#include "data_view.hpp"
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
std::vector<DataView> * DataVector::partition (int n )
{
    std::vector<DataView> * result = new std::vector<DataView> (n);
    int chunk_size = vecData->size() / n;
    for(int i=0;i<n;i++)
    {
        DataView &dv = * new DataView();
        dv.origin = vecData;
        dv.start = chunk_size * i;
        dv.end = dv.start + chunk_size;
        (*result)[i] = dv;
    }
    return result;

}
