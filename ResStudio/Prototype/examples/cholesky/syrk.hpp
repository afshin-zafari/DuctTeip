  string get_name(){return string( "syrk");}
/*----------------------------------------------------------------------------*/
struct SyrkTask : public SuperGlueTaskBase {

  SyrkTask(IDuctteipTask *task_,Handle<Options> &h1,Handle<Options> &h2):SuperGlueTaskBase(task_) {
    register_access(ReadWriteAdd::read,*task_->getSyncHandle());
    registerAccess(ReadWriteAdd::read , h1);
    registerAccess(ReadWriteAdd::write, h2);
    name.assign("syrk");
  }
  void runKernel() {
    
    int N = getAccess(1).getHandle()->block->X_E();
    double *a = getAccess(1).getHandle()->block->getBaseMemory();
    double *b = getAccess(2).getHandle()->block->getBaseMemory();
    assert(a);
    assert(b);
#ifdef USE_MKL    
    int nb = N;
    double DONE=1.0, DMONE=-1.0;
    dsyrk("L", "N", &nb, &nb, &DMONE, a, &nb, &DONE, b, &nb);
#endif
#ifdef USE_OPENBLAS
    dsyrk(CblasColMajor, CblasLower,CblasNoTrans,N,N,-1.0,a,N,1.0,b,N);
#endif
  }
};
