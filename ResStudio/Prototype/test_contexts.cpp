#include "generic_contexts.hpp"

void sample_func2();

void sample_func1(){
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
  sample_func2();
}
/*------------------------------------------------------------*/
void sample_func2(){
  BX(A);
  {
    for (int i=0;i<4; i++){
      BXX(Y,i);
      PRINTF_IND(" Y body %d\n",i);
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
  bool f1=func.compare(func.size()-1,1,string("1"))==0;
  if ( p %2 ==0 && f1)
    return true;
  if ( p %2 ==1 && !f1)
    return true;
  if ( func.compare(string("main"))==0)
    return true;
  
  return false;
}

/*====================================================================*/

int main (int *argc, char **argv){
  printf("Starts\n");
  procs=4;
  for ( me =-1; me < procs; me++){
    printf ("---------------- Proc %d ---------------\n",me);
    BX(MAIN);
    sample_func1();
    EX(MAIN);

    BX(M2);
    EX(M2);
    Glb.resetCtx();
    ctx_mngr.dump();
    ctx_mngr.process();
    ctx_mngr.reset();
  }

  printf("Ends\n");
}
