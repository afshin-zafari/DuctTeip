#include "context.hpp"
#include "chol_fact.hpp"

#include <stdlib.h>




int main (int argc, char * argv[])
{
  int N,Nb,P,p,q,nb,nt,dlb,ipn;
  long st=100,sw=100,sm=1000,to=2000;
  p  = atoi(argv[1]);
  q  = atoi(argv[2]);
  N  = atoi(argv[3]);
  Nb = atoi(argv[4]);
  nb = atoi(argv[5]);
  nt = atoi(argv[6]);
  dlb = atoi(argv[7]);
  ipn= atoi(argv[8]);
  string od;
  if ( argc > 9 ) 
    od = string(argv[9]);
  if ( argc > 10 ) 
    to = atol(argv[10]);
  if ( argc > 11 ) 
    sw = atoi(argv[11]);
  if ( argc > 12 ) 
    sm = atoi(argv[12]);    
  if ( argc > 13 ) 
    st = atoi(argv[13]);    
  if ( dlb ==-1){
    simulation = 1; 
    dlb=0;
  }
printf("ipn:%d\n",ipn);
  
  dtEngine.setSkips(st,sw,sm,to);
  
  P = p * q;

  Config cfg(N,N,Nb,Nb,P,p,q,nb,nb,nt,dlb,ipn,od);
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
  
  Cholesky C(&cfg);

  glbCtx.doPropagation(false);
  dtEngine.doProcess();

  C.generateTasksNoContext();

  dtEngine.finalize();

  if (N < 10000 || dlb)
    C.checkCorrectness();
}
