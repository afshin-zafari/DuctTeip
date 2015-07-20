/*----------------------------------------------------------------------------*/
struct SyrkTask : public Task<Options,3> {
  IDuctteipTask *dt_task ;
  string log_name;
/*----------------------------------------------------------------------------*/
  SyrkTask(IDuctteipTask *task_,Handle<Options> &h1,Handle<Options> &h2):dt_task(task_) {
    registerAccess(ReadWriteAdd::read, *dt_task->getSyncHandle());
    registerAccess(ReadWriteAdd::read, h1);
    registerAccess(ReadWriteAdd::write, h2);
    ostringstream os;
    os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_syrk " ;
    log_name = string("syrk_task");//os.str();
  }
/*----------------------------------------------------------------------------*/
  void run(TaskExecutor<Options> &te) {
    if ( simulation) return;
    
    PRINT_IF(KERNEL_FLAG)("[%ld] : sg_syrk task starts running.A:%s, B:%s\n",pthread_self(),
			  getAccess(1).getHandle()->name,
			  getAccess(2).getHandle()->name);
      int M = getAccess(1).getHandle()->block->X_E();
      int N = M;
      int size = getAccess(1).getHandle()->block->getMemorySize();
      double *a = getAccess(1).getHandle()->block->getBaseMemory();
      double *b = getAccess(2).getHandle()->block->getBaseMemory();
      if (DEBUG_DLB_DEEP) printf("[%ld] r  S: %p %ld %p\n",pthread_self(),a,M*M*sizeof(double),a+M*M);
      if (DEBUG_DLB_DEEP) printf("[%ld]  w S: %p %ld %p\n",pthread_self(),b,M*M*sizeof(double),b+M*M);
      dumpData(a,M,N,'s');
      double key = b[0];
      dumpData(b,M,N,'s');
      if ( config.using_blas){
	// void dsyrk(char uplo, char transa, int n, int k, double alpha, 
	//            double *a, int lda, double beta, double *c, int ldc);
	//	LOG_INFO(LOG_MULTI_THREAD,"a mem:%p, N:%d\n",a,N);
	//	LOG_INFO(LOG_MULTI_THREAD,"b mem:%p, N:%d\n",b,N);
	dsyrk('L','N',N,N,-1.0,a,N,1.0,b,N);
      }
      else{
	for ( int i = 0 ; i < M; i ++){
	  for ( int j =i ; j< M; j++){
	    for ( int k = 0 ; k < M ; k ++) {
	      Mat(b,j,i) -= Mat(a,j,k) *Mat(a,i,k);
	    }
	  }	
	}
      }
      dumpData(b,M,N,'S');
      if (!check_syrk(b,M,N,key) ){
	printf("CalcError in Task:%s,%s\n",dt_task->getName().c_str(),log_name.c_str());
      }
      if (DEBUG_DLB_DEEP) printf("[%ld] : ----------------------------------------\n",pthread_self());
    }
/*----------------------------------------------------------------------------*/
  string get_name(){ return log_name;}
/*----------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
