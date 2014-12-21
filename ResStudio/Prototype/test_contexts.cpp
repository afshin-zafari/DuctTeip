#include "generic_contexts.hpp"
#include "simulate.hpp"

Data x("x"),y("y"),z("z");

void Fx2();

void Fx3(){
  BX(A);
  task(x,y);
  {
    BX(B);
    task(x,z);
    EX(B);
  }
  task(x,y);
  { 
    BX(C);
    task(x,y);
    {
      BX(E);
      task(x,y);
      EX(E);
    }
    task(x,y);
    {
      BX(F);
      task(x,y);
      EX(F);
    }
    task(x,y);
    EX(C);
  }
  task(x,y);
  for ( int i =0;i<3;i++)
    {
      BX(D);
      task(x,y);
      EX(D);
    }
  task(x,y);
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
      task(x,y);
      task(y,z);
      EX(Y);
    }
  }
  EX(A);
}

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

void GenericContext::dataTouched(Data *d){
  touched_data.push_back(d);// to do : push data items only once
}
void GenericContext::printTouchedData(){
  DListIter it;
  printf ("Touched Data : ");
  for(it =touched_data.begin(); 
      it!=touched_data.end(); 
      it++)
    {
      Data *d=*it;
      printf("%s,",d->Name());
    }
  printf("\n");
}
void ContextManager::dataTouched(Data *d){
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

int main (int *argc, char **argv){
  printf("Starts\n");
  procs=4;
  for ( me =-1; me < procs; me++){
    printf ("---------------- Proc %d ---------------\n",me);
    BX(MAIN);
    Fx3();
    EX(MAIN);

    BX(M2);
    EX(M2);
    Glb.resetCtx();
    printf("1\n");
    ctx_mngr.dump();
    printf("1\n");
    ctx_mngr.processAll();
    printf("1\n");
    ctx_mngr.reset();
    printf("1\n");
    x.reset();
    printf("1\n");
    y.reset();
    printf("1\n");
    z.reset();
    printf("1\n");
  }

  printf("Ends\n");
}
