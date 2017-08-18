/*----------------------------------------------------------------------------*/
struct GemmTask : public SuperGlueTaskBase {
  bool b_trans,c_decrease;
/*----------------------------------------------------------------------------*/
  GemmTask(IDuctteipTask *task_ ,
	   Handle<Options> &h1,
	   Handle<Options> &h2,
	   Handle<Options> &h3,
	   bool trans_b=false,
	   bool decrease_c = true):
    SuperGlueTaskBase (task_),
    b_trans(trans_b),
    c_decrease(decrease_c){
        registerAccess(ReadWriteAdd::read , h1);
        registerAccess(ReadWriteAdd::read , h2);
        registerAccess(ReadWriteAdd::write, h3);
    }

/*----------------------------------------------------------------------------*/
    void runKernel() {
      int N = getAccess(1).getHandle()->block->X_E();
      double *a = getAccess(1).getHandle()->block->getBaseMemory();
      double *b = getAccess(2).getHandle()->block->getBaseMemory();
      double *c = getAccess(3).getHandle()->block->getBaseMemory();
      //      char transb = (b_trans)?'T':'N';
      CBLAS_TRANSPOSE  transb = (b_trans)?CblasTrans:CblasNoTrans;
      double alpha = (c_decrease)?-1.0:1.0;
      double beta = 1.0;
      
      //printf("+++++++++++ N in gemm :%d\n",N);
      //printf("+++++++++++ a:%p, b:%p, c:%p\n",a,b,c);
      assert(a);
      assert(b);
      assert(c);
      dgemm(CblasColMajor,CblasNoTrans,transb,N,N,N,alpha,a,N,b,N,beta,c,N);
    }
  /*----------------------------------------------------------------------------*/
};
