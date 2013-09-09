#include "context.hpp"
#include "mat_asm.hpp"
#include "chol_fact.hpp"

#include <stdlib.h>

int main (int argc, char * argv[])
{
  int P=4,p=2,q=2;
  Config cfg(1000,1000,2,2,P,p,q);
  ProcessGrid PG(P,p,q);
  Cholesky C(&cfg);
  DataHostPolicy    hpData    (DataHostPolicy::BLOCK_CYCLIC         , PG );
  TaskHostPolicy    hpTask    (TaskHostPolicy::WRITE_DATA_OWNER     , PG );
  ContextHostPolicy hpContext (ContextHostPolicy::PROC_GROUP_CYCLIC , PG );
  TaskReadPolicy    hpTaskRead(TaskReadPolicy::ALL_GROUP_MEMBERS    , PG );
  TaskAddPolicy     hpTaskAdd (TaskAddPolicy::GROUP_LEADER          , PG );



  glbCtx.setPolicies(&hpData,&hpTask,&hpContext,&hpTaskRead,&hpTaskAdd);
  C.generateTasks();


}
