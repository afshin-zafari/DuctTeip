#include "context.hpp"
#include "mat_asm.hpp"
#include "chol_fact.hpp"

#include <stdlib.h>




int main (int argc, char * argv[])
{
  int Nb,P,p=2,q=2;
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
  TRACE_LOCATION;

  MatrixAssembly MA(&cfg);
  TRACE_LOCATION;
  Cholesky C(&cfg,MA.getOutputData(MatrixAssembly::RBFresult));

  TRACE_LOCATION;
  glbCtx.initPropagateTasks();

  dtEngine.doProcess();// async. if MPI multi threaded is ok.
  TRACE_LOCATION;
  MA.generateTasks();
  //  C.generateTasks();
  TRACE_LOCATION;
  dtEngine.finalize();
  TRACE_LOCATION;
}
