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
	name.assign("gemm");
    }
  string get_name(){return string( "gemm");}
/*----------------------------------------------------------------------------*/
    void runKernel() {
      int N = getAccess(1).getHandle()->block->X_E();
      double *a = getAccess(1).getHandle()->block->getBaseMemory();
      double *b = getAccess(2).getHandle()->block->getBaseMemory();
      double *c = getAccess(3).getHandle()->block->getBaseMemory();
#ifdef USE_OPENBLAS
      //char transb = (b_trans)?'T':'N';
      CBLAS_TRANSPOSE  transb = (b_trans)?CblasTrans:CblasNoTrans;
#endif
#ifdef USE_MKL
      const char *transb = (b_trans)?"T":"N";
#endif
      double alpha = (c_decrease)?-1.0:1.0;
      double beta = 1.0;
      
      //printf("+++++++++++ N in gemm :%d\n",N);
      //printf("+++++++++++ a:%p, b:%p, c:%p\n",a,b,c);
      assert(a);
      assert(b);
      assert(c);
#ifdef USE_MKL
      int nb=N;
      double DONE=1.0, DMONE=-1.0;
      dgemm("N",transb, &nb, &nb, &nb, &alpha, a, &nb, b, &nb, &beta, c, &nb);
#endif
#ifdef USE_OPENBLAS
      dgemm(CblasColMajor,CblasNoTrans,transb,N,N,N,alpha,a,N,b,N,beta,c,N);
#endif
    }
  /*----------------------------------------------------------------------------*/
};
