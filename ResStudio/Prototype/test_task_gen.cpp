#include "context.hpp"
#include "chol_fact.hpp"

#include "ductteip.h"
#include <stdlib.h>


int main (int argc, char * argv[])
{
  DuctTeip_Start(argc,argv);
  
  Cholesky_DuctTeip();

  DuctTeip_Finish();

}
