#include "data_view.hpp"
#include "data_vector.hpp"
#include "data_matrix.hpp"

void DataViewContext::test_01()
{
    DataVector dvec(12,this);
    std::vector<Data*> &vecData = *dvec.vecData;

    std::vector<DataView> &listView = *dvec.partition(2);
    listView[0].register_read();

    std::vector<DataView> &listView2 = *dvec.partition(  4);
    listView2[0].register_read();
    
    for ( int i=0;i<vecData.size(); i++)
    {
        printf("data[%d].read version = %d\n",i,vecData[i]->getReadVersion().getVersion());
    }
}

void DataViewContext::test_02()
{
    DataMatrix dmat(12,12,this);
}
DataViewContext::DataViewContext()
{
}
