#include "context.hpp"
#include "mat_asm.hpp"


int main () 
{
  Config cfg(100,100,4,4,6,2,3);
  MatrixAssembly MA(&cfg);
  DataHostPolicy polColCyclic(MA.getProcessGrid());

  for (  me =0; me<cfg.getProcessors(); me++ ) {
    cout << "\n---- Node " << me << "----" << endl;
    MA.generateTasksByPolicy(me,&polColCyclic);
  }

}
