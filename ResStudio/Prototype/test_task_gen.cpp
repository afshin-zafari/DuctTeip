#include "context.hpp"
#include "chol_fact.hpp"

#include <stdlib.h>




int main (int argc, char * argv[])
{
  int N,Nb,P,p,q,nb,nt=4;
  p  = atoi(argv[1]);
  q  = atoi(argv[2]);
  N  = atoi(argv[3]);
  Nb = atoi(argv[4]);
  nb = atoi(argv[5]);
  
  P = p * q;

  Config cfg(N,N,Nb,Nb,P,p,q,nb,nb,nt);
  ProcessGrid PG(P,p,q);

  DataHostPolicy      hpData    (DataHostPolicy::BLOCK_CYCLIC         , PG );
  TaskHostPolicy      hpTask    (TaskHostPolicy::WRITE_DATA_OWNER     , PG );
  ContextHostPolicy   hpContext (ContextHostPolicy::ALL_ENTER         , PG );
  TaskReadPolicy      hpTaskRead(TaskReadPolicy::ALL_READ_ALL         , PG );
  TaskAddPolicy       hpTaskAdd (TaskAddPolicy::WRITE_DATA_OWNER      , PG );
  TaskPropagatePolicy hpTaskProp(TaskPropagatePolicy::GROUP_LEADER    , PG );

  hpContext.setGroupCount(1,2,1);  // all processors allowed for first level,
                                   // divide them by 2 for second level
                                   // all (from previous level) for third level
  glbCtx.setPolicies(&hpData,&hpTask,&hpContext,&hpTaskRead,&hpTaskAdd,&hpTaskProp);
  glbCtx.setConfiguration(&cfg);
  dtEngine.setConfig(&cfg);

  //MatrixAssembly MA(&cfg);
  //Cholesky C(&cfg,MA.getOutputData(MatrixAssembly::RBFresult));
  Cholesky C(&cfg);

  //glbCtx.initPropagateTasks();
  glbCtx.doPropagation(false);
  dtEngine.doProcess();// async.
  //MA.generateTasks();
  C.generateTasks();
  dtEngine.finalize();
  C.checkCorrectness();
  printf("end of program.\n"); 
}
