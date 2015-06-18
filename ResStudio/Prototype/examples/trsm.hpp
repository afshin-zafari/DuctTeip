struct TrsmTask : public Task<Options, 3> {
  IDuctteipTask *dt_task ;
  string log_name;
  TrsmTask(  IDuctteipTask *task_ ,Handle<Options> &h1,Handle<Options> &h2):  dt_task(task_)  {
    registerAccess(ReadWriteAdd::read, *dt_task->getSyncHandle());
    registerAccess(ReadWriteAdd::read, h1);
    registerAccess(ReadWriteAdd::write, h2);
    ostringstream os;
    os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_trsm ";
    log_name = os.str();
  }
    void run() {
      if ( simulation) return;
      PRINT_IF(KERNEL_FLAG)("[%ld] : sg_trsm task starts running.A:%s, B:%s\n",pthread_self(),
			  getAccess(1).getHandle()->name,
			  getAccess(2).getHandle()->name);
      int M = getAccess(1).getHandle()->block->X_E();
      int N = M;//getAccess(1).getHandle()->block->Y_ES();
      int size = getAccess(1).getHandle()->block->getMemorySize();
      double *a = getAccess(1).getHandle()->block->getBaseMemory();
      double *b = getAccess(2).getHandle()->block->getBaseMemory();
      //      printf("Trsm a:%p n:%d\n",a,N);
      //      printf("Trsm b:%p n:%d\n",b,N);
      dumpData(a,M,N,'t');
      dumpData(b,M,N,'t');
      if (DEBUG_DLB_DEEP) printf("[%ld] r  T: %p %ld %p\n",pthread_self(),a,M*M*sizeof(double),a+M*M);
      if (DEBUG_DLB_DEEP) printf("[%ld]  w T: %p %ld %p\n",pthread_self(),b,M*M*sizeof(double),b+M*M);
#ifdef BLAS
      // void dtrsm(char side, char uplo, char transa, char diag, 
      //            int m, int n, double alpha, double *a, int lda, double *b, int ldb);
      dtrsm('R','L','T','N',N,N,1.0,a,N,b,N);
#else
      for ( int i = 0 ; i < M; i ++){
	for ( int j =0 ; j< M; j++){
	  for ( int k=0;k<i;k++) {
	    Mat(b,j,i) -= Mat(b,j,k) *Mat(a,i,k);
	  }
	  Mat(b,j,i) /= Mat(a,i,i);
	}
      }
#endif
      dumpData(b,M,N,'T');
      if ( ! check_trsm(b,M,N)){
	printf("CalcError in Task:%s,%s\n",dt_task->getName().c_str(),log_name.c_str());
      }
      if (DEBUG_DLB_DEEP) printf("[%ld] : ----------------------------------------\n",pthread_self());
    }
  string get_name(){ return log_name;}
};
