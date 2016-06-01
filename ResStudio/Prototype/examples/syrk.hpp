/*----------------------------------------------------------------------------*/
struct SyrkTask : public SuperGlueTaskBase {

  SyrkTask(IDuctteipTask *task_,Handle<Options> &h1,Handle<Options> &h2):SuperGlueTaskBase(task_) {
    registerAccess(ReadWriteAdd::read , h1);
    registerAccess(ReadWriteAdd::write, h2);
  }
  void runKernel() {
    
    int N = getAccess(1).getHandle()->block->X_E();
    double *a = getAccess(1).getHandle()->block->getBaseMemory();
    double *b = getAccess(2).getHandle()->block->getBaseMemory();
    dsyrk('L','N',N,N,-1.0,a,N,1.0,b,N);
  }
};
