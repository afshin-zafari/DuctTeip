#include "context.hpp"
#include "generic_contexts.hpp"

//#include "simulate.hpp"

//#define dtg_task(a,b) AddTask(this,T,k,a,b);ctx_mngr.dataTouched(a);ctx_mngr.dataTouched(b);
#define dtg_task(a,b) ctx_mngr.dataTouched(a);ctx_mngr.dataTouched(b);


/*===================================================================================*/
/*====                                                                           ====*/
/*====                         DTG Context                                       ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*===================================================================================*/
class DTG:public Context
{
private:
  IData *x,*y,*z;
  IData *a,*b,*c,*d,*e,*f;
  int k,Nb,N;
  char T[2];
  Config *cfg;
public :
  DTG(int n,int NB,Config *cf){
    cfg = cf;
    k=1;
    Nb=NB;
    N=n;
    T[0]='A';
    T[1]=0;
    configureData(&x,"x");
    configureData(&y,"y");
    configureData(&z,"z");

    configureData(&a,"a");
    configureData(&b,"b");
    configureData(&c,"c");
    configureData(&d,"d");
    configureData(&e,"e");
    configureData(&f,"f");

  }

  /*--------------------------------------------------------------------*/
  void configureData(IData **d,char *s){
    *d= new IData (s,N,N,this);
    (*d)->setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
    (*d)->setLocalNumBlocks(cfg->getXLocalBlocks(),cfg->getXLocalBlocks());
    (*d)->setPartition( Nb,Nb);

  }
  void runKernels(IDuctteipTask *task ){}
  string getTaskName(unsigned long) {return string("a");}


  /*--------------------------------------------------------------------*/
  void Fx3(){
    dtg_task(z,z);
    BX(A);
    dtg_task(x,a);
    {
      BX(A_B);
      dtg_task(x,b);
      EX(A_B);
    }
    dtg_task(y,a);
    { 
      BX(C);
      dtg_task(x,c);
      {
	BX(E);
	dtg_task(x,e);
	EX(E);
      }
      dtg_task(x,c);
      {
	BX(F);
	dtg_task(x,f);
	EX(F);
      }
      dtg_task(x,c);
      EX(C);
    }
    dtg_task(x,a);
    for ( int i =0;i<1;i++)
      {
	BX(D);
	dtg_task(x,d);
	EX(D);
      }
    dtg_task(x,a);
    EX(A);
    dtg_task(z,y);
    Fx2();
  }
  /*------------------------------------------------------------*/
  void Fx2(){
    BX(A);
    dtg_task(a,y);
    {
      for (int i=0;i<4; i++){
	BXX(Y,i);
	dtg_task(x,y);
	dtg_task(y,z);
	EX(Y);
	dtg_task(a,x);
      }
    }
    dtg_task(a,z);
    EX(A);
  }
};
/*==============================================================================*/

/*------------------------------------------------------------*/
bool ctx_enter(GenericContext *x,int p){
  string func= x->getFunc();
  if ( p==-2) p = me;
  if ( p==-1)return true;
  bool f1=func.compare(func.size()-1,1,string("3"))==0;
  if ( p %2 ==0 && f1)
    return true;
  if ( p %2 ==1 && !f1)
    return true;
  if ( func.compare(string("main"))==0)
    return true;
  
  return false;
}

/*====================================================================*/
DTG *dtg;

int main (int argc, char **argv){
  int N,Nb,P,p,q,nb,nt,dlb;
  long st=100,sw=100,sm=1000,to=2000;
  p  = atoi(argv[1]);
  q  = atoi(argv[2]);
  N  = atoi(argv[3]);
  Nb = atoi(argv[4]);
  nb = atoi(argv[5]);
  nt = atoi(argv[6]);
  dlb = atoi(argv[7]);
  if ( argc > 8 ) 
    to = atol(argv[8]);
  if ( argc > 9 ) 
    sw = atoi(argv[9]);
  if ( argc > 10 ) 
    sm = atoi(argv[10]);    
  if ( argc > 11 ) 
    st = atoi(argv[11]);    
  if ( dlb ==-1){
    simulation = 1; 
    dlb=0;
  }
  
  dtEngine.setSkips(st,sw,sm,to);
  
  P = p * q;

  Config cfg(N,N,Nb,Nb,P,p,q,nb,nb,nt,dlb);
  ProcessGrid PG(P,p,q);
  DataHostPolicy      hpData    (DataHostPolicy::BLOCK_CYCLIC         , PG );
  TaskHostPolicy      hpTask    (TaskHostPolicy::WRITE_DATA_OWNER     , PG );
  ContextHostPolicy   hpContext (ContextHostPolicy::ALL_ENTER         , PG );
  TaskReadPolicy      hpTaskRead(TaskReadPolicy::ALL_READ_ALL         , PG );
  TaskAddPolicy       hpTaskAdd (TaskAddPolicy::WRITE_DATA_OWNER      , PG );
  TaskPropagatePolicy hpTaskProp(TaskPropagatePolicy::GROUP_LEADER    , PG );

  hpContext.setGroupCount(1,2,1);
  glbCtx.setPolicies(&hpData,&hpTask,&hpContext,&hpTaskRead,&hpTaskAdd,&hpTaskProp);
  glbCtx.setConfiguration(&cfg);
  dtEngine.setConfig(&cfg);
  

  glbCtx.doPropagation(false);
  dtEngine.doProcess();


  printf("Starts\n");
  dtg = new DTG(N,Nb,&cfg);
  procs=P;
  printf ("---------------- Proc %d ---------------\n",me);

  BX(MAIN);  
  dtEngine.barrier();
  dtg->Fx3();
  EX(MAIN);
  
  BX(M2);
  EX(M2);
  
  printf("Ends\n");
  dtEngine.finalize();
  
}
