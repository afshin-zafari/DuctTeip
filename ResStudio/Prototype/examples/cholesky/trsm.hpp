struct TrsmTask : public SuperGlueTaskBase {
  TrsmTask(  IDuctteipTask *task_ ,Handle<Options> &h1,Handle<Options> &h2):  SuperGlueTaskBase(task_)  {
    registerAccess(ReadWriteAdd::read , h1);
    registerAccess(ReadWriteAdd::write, h2);
    name.assign("trsm");
  }
  string get_name(){return string( "trsm");}
  void runKernel() {
    int N = getAccess(1).getHandle()->block->X_E();
    double *a = getAccess(1).getHandle()->block->getBaseMemory();
    double *b = getAccess(2).getHandle()->block->getBaseMemory();
    assert(a);
    assert(b);
   
#ifdef USE_MKL
    double DONE=1.0;
    int nb = N;
    dtrsm("R", "L", "T", "N", &nb, &nb, &DONE, a, &nb, b, &nb);
#endif
#ifdef USE_OPENBLAS
    dtrsm(CblasColMajor,CblasRight,CblasLower,CblasTrans,CblasNonUnit,N,N,1.0,a,N,b,N);
#endif
  }
};
