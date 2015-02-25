#ifndef __DUCTTEIP_H__
#define  __DUCTTEIP_H__

#include "context.hpp"
#define DuctTeip_Start(a,b) dtEngine.start(a,b)
#define DuctTeip_Finish() dtEngine.finalize()

extern Config config;

void engine::start ( int argc , char **argv){
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
    if ( argc > 9 ) 
      to = atol(argv[9]);
    if ( argc > 10 ) 
      sw = atoi(argv[10]);
    if ( argc > 11 ) 
      sm = atoi(argv[11]);    
    if ( argc > 12 ) 
      st = atoi(argv[12]);    
    if ( dlb ==-1){
      simulation = 1; 
      dlb=0;
    }
    P = p * q;
    printf("ipn:%d, P:%d,p:%d,q:%d\n",ipn,P,p,q);
    printf("N:%d, Nb:%d,nb:%d,nt:%d dlb:%d\n",N,Nb,nb,nt,dlb);
  
    setSkips(st,sw,sm,to);
  

    config.setParams(N,N,Nb,Nb,P,p,q,nb,nb,nt,dlb,ipn);
    ProcessGrid *_PG = new ProcessGrid(P,p,q);
    ProcessGrid &PG=*_PG;

    DataHostPolicy      *hpData    = new DataHostPolicy    (DataHostPolicy::BLOCK_CYCLIC         , PG );
    TaskHostPolicy      *hpTask    = new TaskHostPolicy    (TaskHostPolicy::WRITE_DATA_OWNER     , PG );
    ContextHostPolicy   *hpContext = new ContextHostPolicy (ContextHostPolicy::ALL_ENTER         , PG );
    TaskReadPolicy      *hpTaskRead= new TaskReadPolicy    (TaskReadPolicy::ALL_READ_ALL         , PG );
    TaskAddPolicy       *hpTaskAdd = new TaskAddPolicy     (TaskAddPolicy::WRITE_DATA_OWNER      , PG );
    TaskPropagatePolicy *hpTaskProp=new TaskPropagatePolicy(TaskPropagatePolicy::GROUP_LEADER    , PG );

    hpContext->setGroupCount(1,2,1);  // all processors allowed for first level,
    // divide them by 2 for second level
    // all (from previous level) for third level
    glbCtx.setPolicies(hpData,hpTask,hpContext,hpTaskRead,hpTaskAdd,hpTaskProp);
    glbCtx.setConfiguration(&config);
    setConfig(&config);
    glbCtx.doPropagation(false);
  
    doProcess();
  }

#endif // __DUCTTEIP_H__
