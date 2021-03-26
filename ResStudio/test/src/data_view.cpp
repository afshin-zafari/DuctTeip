#include "data_view.hpp"
#include "data_vector.hpp"
void DataView::register_read()
{
    for ( int i =start; i< end; i++)
    {
        Data &d = *origin->at(i);
        d.addToVersion(IData::AccessType::READ , 1);
    }
}
std::vector<DataView> * DataViewContext::partition ( std::vector<Data*> * data_vector, int n )
{
    std::vector<DataView> * result = new std::vector<DataView> (n);
    int chunk_size = data_vector->size() / n;
    for(int i=0;i<n;i++)
    {
        DataView &dv = * new DataView();
        dv.origin = data_vector;
        dv.start = chunk_size * i;
        dv.end = dv.start + chunk_size;
        (*result)[i] = dv;
    }
    return result;

}
void DataViewContext::test_01()
{
    DataVector dvec(12,this);
    std::vector<Data*> &vecData = dvec.vecData;

    std::vector<DataView> &listView = *partition( &vecData, 2);
    listView[0].register_read();

    std::vector<DataView> &listView2 = *partition( &vecData, 4);
    listView2[0].register_read();
    
    for ( int i=0;i<vecData.size(); i++)
    {
        printf("data[%d].read version = %d\n",i,vecData[i]->getReadVersion().getVersion());
    }
}
DataViewContext::DataViewContext()
{
}
