#include <mpi.h>
#include <stdio.h>
#include "ductteip.hpp"

#include "data_view.hpp"
int main (int argc, char * argv[])
{
    DuctTeip_Start(argc,argv,true);
    DataViewContext dv;
    DuctTeip_Finish();
}