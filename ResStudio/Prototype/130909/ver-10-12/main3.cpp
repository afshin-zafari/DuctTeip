#include "context.hpp"
#include "mat_asm.hpp"


int main () 
{
  Config cfg(100,100,4,4,6,2,3);
  MatrixAssembly MA(&cfg);

  for (  me =0; me<cfg.getProcessors(); me++ ) {
    cout << "\n---- Node " << me << "----" << endl;
    MA.generateTasks(me);
  }

}
