#include "data_view.hpp"
void DataView::register_read()
{
    printf("2-1\n");
    for ( int i =start; i< end; i++)
    {
        printf("2-11\n");
        Data &d = *origin->at(i);
        printf("read version: %d\n",d.getReadVersion().getVersion());
        d.addToVersion(IData::AccessType::READ , 1);
        printf("read version: %d\n",d.getReadVersion().getVersion());
        printf("%s\n",d.getReadVersion().dumpString().c_str());
        
    }
    printf("2-2\n");
}
std::vector<DataView> * DataViewContext::partition ( std::vector<Data*> * data_vector, int n )
{
    printf("1-1\n");
    std::vector<DataView> * result = new std::vector<DataView> (n);
    int chunk_size = data_vector->size() / n;
    for(int i=0;i<n;i++)
    {
        printf("1-11\n");
        DataView &dv = * new DataView();
        dv.origin = data_vector;
        dv.start = chunk_size * i;
        dv.end = dv.start + chunk_size;
        (*result)[i] = dv;
    }
    printf("1-2\n");
    return result;
}
DataViewContext::DataViewContext()
{
    std::vector<Data*> &vecData =* new std::vector<Data*> (10);
    int b = 2;
    int n = 2;
    for( int i=0; i<  vecData.size(); i++ )
    {
        Data *d = new Data("d",12,12,this);
        d->setDataHandle( d->getParent()->createDataHandle(d));
        d->setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
        d->setLocalNumBlocks(n,n);

        d->setPartition(b,b) ;
        addInOutData(d);
        vecData[i] = d;
    }
    printf("1\n");
    std::vector<DataView> &listView = *partition( &vecData, 2);
    printf("2\n");
    listView[0].register_read();
    printf("3\n");
    for ( int i=0;i<vecData.size(); i++)
    {
        printf("data[%d].read version = %d\n",i,vecData[i]->getReadVersion().getVersion());
    }
}
