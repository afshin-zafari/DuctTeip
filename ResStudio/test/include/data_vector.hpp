#pragma once
#include "ductteip.hpp"
class DataView;
class DataVector
{
    public:
    DataVector(int n , Context *) ;
    std::vector<Data*> *vecData;    
    std::vector<DataView> * partition (  int n );


};