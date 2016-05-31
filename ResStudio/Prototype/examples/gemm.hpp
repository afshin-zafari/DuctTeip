/*----------------------------------------------------------------------------*/
struct GemmTask : public BasicDTTask {
  bool b_trans,c_decrease;
/*----------------------------------------------------------------------------*/
  GemmTask(IDuctteipTask *task_ ,
	   Handle<Options> &h1,
	   Handle<Options> &h2,
	   Handle<Options> &h3,
	   bool trans_b=false,
	   bool decrease_c = true):
    BasicDTTask (task_),
    b_trans(trans_b),
    c_decrease(decrease_c){
        registerAccess(ReadWriteAdd::read , h1);
        registerAccess(ReadWriteAdd::read , h2);
        registerAccess(ReadWriteAdd::write, h3);
    }

/*----------------------------------------------------------------------------*/
    void runKernel(TaskExecutor<Options> &te) {
      int N = getAccess(1).getHandle()->block->X_E();
      double *a = getAccess(1).getHandle()->block->getBaseMemory();
      double *b = getAccess(2).getHandle()->block->getBaseMemory();
      double *c = getAccess(3).getHandle()->block->getBaseMemory();
      char transb = (b_trans)?'T':'N';
      double alpha = (c_decrease)?-1.0:1.0;
      double beta = 1.0;
      printf("+++++++++++ N in gemm :%d\n",N);
      printf("+++++++++++ a:%p, b:%p, c:%p\n",a,b,c);      
      dgemm('N',transb,N,N,N,alpha,a,N,b,N,beta,c,N);
    }
  /*----------------------------------------------------------------------------*/
};
