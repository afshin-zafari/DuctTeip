#include "context.hpp"
#include "mat_asm.hpp"
#include <stdlib.h>
int main (int argc, char * argv[]) 
{
  version = 11;
  int p ;//= atoi(argv[1]);
  for ( p = 2; p < 10; p++){
    int P = p*p;
    int Nb = 4* p;
    printf("P=%d,p=%d,Nb=%d\n",P,p,Nb);
    Config cfg(1000,1000,Nb,Nb,P,p,p);
    MatrixAssembly *MA= new MatrixAssembly(&cfg);
    DataHostPolicy polColCyclic(MA->getProcessGrid());
  
    for (  me =0; me<cfg.getProcessors(); me++ ) {
      cout << "\n---- Node " << me << "----" << endl;
      glbCtx.setID(0);
      glbCtx.resetCounters();
      MA->resetVersions();
      MA->generateTasksWithSummary(me,&polColCyclic);
      glbCtx.dumpStatistics(&cfg);
    }
    MA->resetVersions();
    glbCtx.setID(0);
    glbCtx.resetCounters();
    me = -1; 
    MA->generateTasksSingleNode();
    glbCtx.dumpStatistics(&cfg);
    delete MA;
  }
}
