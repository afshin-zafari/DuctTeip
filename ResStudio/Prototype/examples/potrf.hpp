  void   Ductteip_POTRF_Kernel(ElementType_Data &a){
    int N=a.N;
    if ( config.column_major){
      if ( config.using_blas){
	int info;
	dpotrf('L',N,a.memory,N,&info);
      }
      else{
	for ( int i = 0 ; i < N; i ++){
	  for ( int k = 0 ; k < i ; k ++) {
	    for ( int j =i ; j< N; j++){
	      Mat(a,j,i) -= Mat(a,j,k) * Mat(a,i,k);
	    }
	  }	
	  a[i*N+i] = sqrt(a[i*N+i]);
	  for ( int j =0 ; j< N; j++){
	    Mat(a,j,i) /= Mat(a,i,i);
	  }
	}
      }
    }
    else{
      /*Row major ordering implementation...*/
    }
  }
/*--------------------------------------------------------------------------------*/
class Potrf2Task:public  SGTask4DTKernel{
private:
public:
  Potrf2Task(IDuctteipTask *task_,Handle<Options> &h1):SGTask4DTKernel(task_) {
      registerAccess(ReadWriteAdd::write, h1);
      ostringstream os;
      os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_potrf ";
      log_name = os.str();
  }
  void run() {
    if ( simulation) return;
    int N;
    ElementType_Data a(this,0,N,N);
    Ductteip_POTRF_Kernel(a);
    return;
  }
};
/*--------------------------------------------------------------------------------*/
  struct PotrfTask : public Task<Options, -1> {
    IDuctteipTask *dt_task ;
    string log_name;
    PotrfTask(IDuctteipTask *task_,Handle<Options> &h1):dt_task(task_) {
      registerAccess(ReadWriteAdd::read, *dt_task->getSyncHandle());
      registerAccess(ReadWriteAdd::write, h1);
      ostringstream os;
      os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_potrf ";
      log_name = os.str();
    }
    void run() {
      if ( simulation) return;
      int N;
      ElementType_Data a(this,0,N,N);
      Ductteip_POTRF_Kernel(a);
      return;
      dumpData(a.memory,N,N,'p');
      if ( config.column_major){
	if ( config.using_blas){
	  int info;
	  dpotrf('L',N,a.memory,N,&info);
	}
	else{
	  for ( int i = 0 ; i < N; i ++){
	    for ( int k = 0 ; k < i ; k ++) {
	      for ( int j =i ; j< N; j++){
		Mat(a,j,i) -= Mat(a,j,k) * Mat(a,i,k);
	      }
	    }	
	    a[i*N+i] = sqrt(a[i*N+i]);
	    for ( int j =0 ; j< N; j++){
	      Mat(a,j,i) /= Mat(a,i,i);
	    }
	  }
	}
      }
      else{
	/*Row major ordering implementation...*/
      }
      dumpData(a.memory,N,N,'P');
      if ( !check_potrf(a.memory,N,N) ){
	printf("CalcError in Task:%s,%s\n",dt_task->getName().c_str(),log_name.c_str());
      }
      if (DEBUG_DLB_DEEP) 
	printf("[%ld] : ----------------------------------------\n",pthread_self());
    }
  string get_name(){ return log_name;}
};
/*----------------------------------------------------------------------------*/

