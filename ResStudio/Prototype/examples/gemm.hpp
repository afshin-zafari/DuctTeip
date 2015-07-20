/*----------------------------------------------------------------------------*/
struct GemmTask : public Task<Options, 4> {
  IDuctteipTask *dt_task ;
  bool b_trans,c_decrease;
  string log_name;
/*----------------------------------------------------------------------------*/
  GemmTask(IDuctteipTask *task_ ,
	   Handle<Options> &h1,
	   Handle<Options> &h2,
	   Handle<Options> &h3,
	   bool trans_b=false,
	   bool decrease_c = true):
    dt_task(task_),
    b_trans(trans_b),
    c_decrease(decrease_c){
    //        printf("SG gemm task ctor 1\n");
        registerAccess(ReadWriteAdd::read , *dt_task->getSyncHandle());
        registerAccess(ReadWriteAdd::read , h1);
        registerAccess(ReadWriteAdd::read , h2);
        registerAccess(ReadWriteAdd::write, h3);
	//        printf("SG gemm task ctor 2\n");
	ostringstream os;
	os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_gemm ";
	log_name = os.str();
    }

/*----------------------------------------------------------------------------*/
    void run(TaskExecutor<Options> &te) {
      if(simulation) return;
      PRINT_IF(KERNEL_FLAG)("[%ld] : sg_gemm task starts running.A:%s,B:%s,C:%s\n",pthread_self(),
			    getAccess(1).getHandle()->name,
			    getAccess(2).getHandle()->name,
			    getAccess(3).getHandle()->name );

      int M = getAccess(1).getHandle()->block->X_E();
      int N = M;
      int size = getAccess(1).getHandle()->block->getMemorySize();
      double *a = getAccess(1).getHandle()->block->getBaseMemory();
      double *b = getAccess(2).getHandle()->block->getBaseMemory();
      double *c = getAccess(3).getHandle()->block->getBaseMemory();
      if (DEBUG_DLB_DEEP) printf("[%ld] r  G: %p %ld %p\n",pthread_self(),a,M*M*sizeof(double),a+M*M);
      if (DEBUG_DLB_DEEP) printf("[%ld] r  G: %p %ld %p\n",pthread_self(),b,M*M*sizeof(double),b+M*M);
      if (DEBUG_DLB_DEEP) printf("[%ld]  w G: %p %ld %p\n",pthread_self(),c,M*M*sizeof(double),c+M*M);
      dumpData(a,M,N,'g');
      dumpData(b,M,N,'g');
      double key=c[0];
      dumpData(c,M,N,'g');
      TimeUnit t = getTime();
      if(config.using_blas){
	// void dgemm(char transa, char transb, int m, int n, int k, 
	//            double alpha, double *a, int lda, double *b, int ldb, double beta, double *c, int ldc);
	char transb = (b_trans)?'T':'N';
	double alpha = (c_decrease)?-1.0:1.0;
	double beta = 1.0;
	//	LOG_INFO(LOG_MULTI_THREAD,"a mem:%p, N:%d\n",a,N);
	//	LOG_INFO(LOG_MULTI_THREAD,"b mem:%p, N:%d\n",b,N);
	//	LOG_INFO(LOG_MULTI_THREAD,"c mem:%p, N:%d\n",c,N);
	dgemm('N',transb,N,N,N,alpha,a,N,b,N,beta,c,N);
      }
      else{
	for ( int i = 0 ; i < M; i ++){
	  for ( int j =0 ; j< M; j++){
	    for ( int k =0 ; k< M; k++){
	      if ( c_decrease){
		if ( b_trans){
		  Mat(c,i,j) -= Mat(a,i,k) * Mat(b,j,k);
		}
		else{
		  Mat(c,i,j) -= Mat(a,i,k) * Mat(b,k,j);
		}
	      }
	      else{
		if ( b_trans){
		  Mat(c,i,j) += Mat(a,i,k) * Mat(b,j,k);
		}
		else{
		  Mat(c,i,j) += Mat(a,i,k) * Mat(b,k,j);
		}
	      }
	    }
	  }
	}
      }
      dumpData(c,M,N,'G');
      if (! check_gemm(c,M,N,key) ) {
	printf("CalcError in Task:%s,%s\n",dt_task->getName().c_str(),log_name.c_str());
      }
      if (DEBUG_DLB_DEEP) printf("[%ld] : ----------------------------------------\n",pthread_self());
    }
/*----------------------------------------------------------------------------*/
  string get_name(){ return log_name;}
/*----------------------------------------------------------------------------*/
};
