#include "context.hpp"
#include "chol_fact.hpp"

#include "ductteip.h"
#include <stdlib.h>


int main (int argc, char * argv[])
{
  DuctTeip_Start(argc,argv);
  
  Cholesky C(&config);

  dtEngine.doProcess();

  C.generateTasksNoContext();

  DuctTeip_Finish();

  if (config.N < 10000 || config.dlb)
    C.checkCorrectness();
}
