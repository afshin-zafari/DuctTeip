#include "context.hpp"
#include "mat_asm.hpp"
#include "chol_fact.hpp"

#include <stdlib.h>

int main (int argc, char * argv[])
{
  int Nb,P=12,p=2,q=6;
  Nb = atoi(argv[1]);
  P  = atoi(argv[2]);
  p = P/q;

  Config cfg(1000,1000,Nb,Nb,P,p,q);
  ProcessGrid PG(P,p,q);

  DataHostPolicy      hpData    (DataHostPolicy::BLOCK_CYCLIC         , PG );
  TaskHostPolicy      hpTask    (TaskHostPolicy::WRITE_DATA_OWNER     , PG );
  ContextHostPolicy   hpContext (ContextHostPolicy::PROC_GROUP_CYCLIC , PG );
  TaskReadPolicy      hpTaskRead(TaskReadPolicy::ALL_GROUP_MEMBERS    , PG );
  TaskAddPolicy       hpTaskAdd (TaskAddPolicy::NOT_OWNER_CYCLIC      , PG );
  TaskPropagatePolicy hpTaskProp(TaskPropagatePolicy::ALL_CYCLIC      , PG );

  hpContext.setGroupCount(1,2,1);  // all processors allowed for first level,
                                   // divide them by 2 for second level
                                   // all (from previous level) for third level
  glbCtx.setPolicies(&hpData,&hpTask,&hpContext,&hpTaskRead,&hpTaskAdd,&hpTaskProp);
  glbCtx.setConfiguration(&cfg);
  Cholesky C(&cfg);
  glbCtx.setTaskCount(C.countTasks() ) ;
  for ( me=0;me<P;me++)
  {
    glbCtx.resetCounters();
    printf("--------------@Node : %d -------------\n",me);
    C.generateTasks();
    glbCtx.dumpStatistics(&cfg);
    C.resetVersions();
  }


}