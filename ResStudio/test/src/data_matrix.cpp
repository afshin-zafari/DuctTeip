#include "data_matrix.hpp"
DataMatrix::DataMatrix(int m , int n, Context * context)
{

    columns = new std::vector<DataVector*>(m);
    for(int i=0; i<columns->size(); i++)
    {
        columns->at(i) = new DataVector(n,context);
    }

}