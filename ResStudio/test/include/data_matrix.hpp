#include "ductteip.hpp"
#include "data_vector.hpp"
#include <vector>
class DataMatrix
{
    public:
    DataMatrix(int,int, Context *);
    std::vector<DataVector *> *columns;
};