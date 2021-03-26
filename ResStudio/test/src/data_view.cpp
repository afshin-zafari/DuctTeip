#include "data_view.hpp"
void DataView::register_read()
{
    for ( int i =start; i< end; i++)
    {
        Data &d = *origin->at(i);
        d.addToVersion(IData::AccessType::READ , 1);
    }
}

