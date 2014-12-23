#include "context.hpp"
#include "generic_contexts.hpp"

//#include "simulate.hpp"

#define dtg_task(a,b) AddTask(this,T,k,a,b);ctx_mngr.dataTouched(a);ctx_mngr.dataTouched(b);


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
    BX(A);
    dtg_task(x,a);
    {
      BX(B);
      dtg_task(x,b);
      EX(B);
    }
    dtg_task(x,a);
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
    for ( int i =0;i<3;i++)
      {
	BX(D);
	dtg_task(x,d);
	EX(D);
      }
    dtg_task(x,a);
    EX(A);
    Fx2();
  }
  /*------------------------------------------------------------*/
  void Fx1(){
    BX(A);
    PRINTF_IND(" A body 1\n");
    {
      BX(B);
      PRINTF_IND(" B body\n");
      EX(B);
    }
    PRINTF_IND(" A body 2\n");
    { 
      BX(C);
      PRINTF_IND(" C body 1\n");
      {
	BX(E);
	PRINTF_IND(" E body\n");
	EX(E);
      }
      PRINTF_IND(" C body 2\n");
      {
	BX(F);
	PRINTF_IND(" F body\n");
	EX(F);
      }
      PRINTF_IND(" C body 3\n");
      EX(C);
    }
    PRINTF_IND(" A body 3\n");
    for ( int i =0;i<3;i++)
      {
	BX(D);
	PRINTF_IND(" D body\n");
	EX(D);
      }
    PRINTF_IND(" A body 4\n");
    EX(A);
    Fx2();
  }
  /*------------------------------------------------------------*/
  void Fx2(){
    BX(A);
    {
      for (int i=0;i<4; i++){
	BXX(Y,i);
	PRINTF_IND(" Y body %d\n",i);
	dtg_task(x,y);
	dtg_task(y,z);
	EX(Y);
      }
    }
    EX(A);
  }
};
/**/
DTG *dtg;

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

void GenericContext::dataTouched(IData *d){
  touched_data.push_back(d);// to do : push data items only once
}
void GenericContext::printTouchedData(){
  DListIter it;
  PRINTF_IND ("   Touched Data : ");
  for(it =touched_data.begin(); 
      it!=touched_data.end(); 
      it++)
    {
      IData *d=*it;
      printf("%s,",d->getName().c_str());
    }
  printf("\n");
}
void ContextManager::dataTouched(IData *d){
  ContextNode *cn=getActiveCtx();
  if ( cn == NULL ) {
    printf("ActvCtx NULL \n");
    return;
  }
  cn->ctx->dataTouched(d);
}
void ContextManager::printTouchedData(){
  ContextNode *cn=getActiveCtx();
  if ( cn == NULL ) {
    printf("ActvCtx NULL \n");
    return;
  }
  cn->ctx->printTouchedData();
}
/*====================================================================*/

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
    dtg->Fx3();
    EX(MAIN);

    BX(M2);
    EX(M2);
    /*
    x.reset();
    y.reset();
    z.reset();
    */

  printf("Ends\n");
  dtEngine.finalize();

}
