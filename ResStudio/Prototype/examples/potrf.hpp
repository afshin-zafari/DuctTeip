#include <acml.h>
/*--------------------------------------------------------------------------------*/
  struct PotrfTask : public SuperGlueTaskBase {
    PotrfTask(IDuctteipTask *task_,Handle<Options> &h1):SuperGlueTaskBase(task_) {
      registerAccess(ReadWriteAdd::write, h1);
    }
    void runKernel(TaskExecutor<Options> &te) {
      int info,N= getAccess(1).getHandle()->block->X_E();
      double *a = getAccess(1).getHandle()->block->getBaseMemory();
      dpotrf('L',N,a,N,&info);
    }
};
/*----------------------------------------------------------------------------*/

