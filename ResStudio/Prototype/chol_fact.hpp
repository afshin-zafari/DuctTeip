#ifndef __CHOL_FACT_HPP__
#define __CHOL_FACT_HPP__

#include "context.hpp"
#include "math.h"

//#define ROW_MAJOR 1
#ifdef ROW_MAJOR
# define Mat(mat,i,j) mat[i*N+j]
#else
# define Mat(mat,i,j) mat[j*N+i]
#endif

#ifdef BLAS
#include <acml.h>
#endif

#define DEBUG_DLB_DEEP 0


bool check_gemm(double *d,int M, int N,double key){
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      if ((key-M+j) != d[i+j*M] ) {
	printf("g[%ld]# key:%lf\n",pthread_self(),key);
	return false;
      }
    }
  }
  return true;
}


bool check_syrk(double *d,int M, int N,double key){
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      if ( i == j ) {
	if ((key-M+j) != d[i+j*M]){
	  printf("s[%ld]# key:%lf\n",pthread_self(),key);
	  return false;
	}
      }
      else{
	if ( i > j ) {
	  if ((key-M-2+j) != d[i+j*M] ) {
	    printf("s[%ld]# key:%lf\n",pthread_self(),key);
	    return false;
	  }
	}
      }
    }
  }
  return true;
}

bool check_trsm(double *d,int M, int N){
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      if (d[i+j*M] != -1 ) {
	printf("t[%ld]# key:%lf\n",pthread_self(),d[i+j*M]);
	return false;
      }
    }
  }
  return true;
}
bool check_potrf(double *d,int M, int N){
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      if (i == j ){
	if (d[i+j*M] != 1 ){
	  printf("s[%ld]# key:%lf\n",pthread_self(),d[i+j*M]);
	  return false;
	}
      }
      else{
	if ( i>j){
	  if (d[i+j*M] != -1 ) {
	    printf("p[%ld]# key:%lf\n",pthread_self(),d[i+j*M]);
	    return false;
	  }
	}
      }
    }
  }
  return true;
}
void dumpData(double *d,int M,int N,char t=' '){
  return;
  if (0 && !DEBUG_DLB_DEEP)
    return;
  if ( 0 && DUMP_FLAG)
    return ;
  char s[1000]="";
  printf("[%ld]* %c: %p %ld %p\n",pthread_self(),t,d,M*M*sizeof(double),d+M*M);
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      sprintf (s+j*5," %3.0lf ",d[i+j*N]);
    }
    printf("[%ld] : %s\n",pthread_self(),s);
  }
  printf("[%ld] : ----------------------------------------\n",pthread_self());
}
/*----------------------------------------------------------------------------*/
struct SyncTask : public Task<Options, -1> {
  IDuctteipTask *dt_task ;
  string log_name;
  SyncTask(IDuctteipTask *task_):dt_task(task_) {
    PRINT_IF(0)("sync task ctor,task:%s\n",dt_task->getName().c_str());
        registerAccess(ReadWriteAdd::write, *dt_task->getSyncHandle());
	ostringstream os;
	os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_sync ";
	log_name = os.str();
    }
  SyncTask(Handle<Options> **h,int M,int N,IDuctteipTask *task_):dt_task(task_) {
    for(int i =0;i<M;i++)
      for (int j=0;j<N;j++)
        registerAccess(ReadWriteAdd::write, h[i][j]);
    printf("sync on all handles:%d,%d for:%s\n",M,N,dt_task->getName().c_str());
    }
    void run() {  }
    ~SyncTask(){
      PRINT_IF(KERNEL_FLAG)("[%ld] : sg_sync task for:%s starts running.\n",pthread_self(),
			    dt_task->getName().c_str());
      dt_task->setFinished(true);
    }
  string get_name(){ return log_name;}
  //string getName(){ return log_name;}
};
/*----------------------------------------------------------------------------*/
struct PotrfTask : public Task<Options, 2> {
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
      PRINT_IF(KERNEL_FLAG)("[%ld] : sg_potrf task starts running. %s\n",pthread_self(),
			    getAccess(0).getHandle()->name);
      int r = pthread_mutex_trylock(dt_task->getTaskFinishMutex()) ;
      int M = getAccess(1).getHandle()->block->X_E();
      int N = M;
      int size = getAccess(1).getHandle()->block->getMemorySize();
      double *a = getAccess(1).getHandle()->block->getBaseMemory();
      if (DEBUG_DLB_DEEP) printf("[%ld] rw P: %p %ld %p\n",pthread_self(),a,M*M*sizeof(double),a+M*M);
      dumpData(a,M,N,'p');
#ifdef BLAS
      int info;
      dpotrf('L',M,a,M,&info);
#else
      for ( int i = 0 ; i < M; i ++){
	for ( int k = 0 ; k < i ; k ++) {
	  for ( int j =i ; j< M; j++){
	    Mat(a,j,i) -= Mat(a,j,k) * Mat(a,i,k);
	  }
	}	
	a[i*N+i] = sqrt(a[i*N+i]);
	for ( int j =0 ; j< M; j++){
	  Mat(a,j,i) /= Mat(a,i,i);
	}
      }
#endif
      dumpData(a,M,N,'P');
      if ( !check_potrf(a,M,N) ){
	printf("CalcError in Task:%s,%s\n",dt_task->getName().c_str(),log_name.c_str());
      }
      if (DEBUG_DLB_DEEP) 
	printf("[%ld] : ----------------------------------------\n",pthread_self());
    }
  string get_name(){ return log_name;}
};
/*----------------------------------------------------------------------------*/
struct SyrkTask : public Task<Options, 3> {
  IDuctteipTask *dt_task ;
  string log_name;
  SyrkTask(IDuctteipTask *task_,Handle<Options> &h1,Handle<Options> &h2):dt_task(task_) {
    registerAccess(ReadWriteAdd::read, *dt_task->getSyncHandle());
    registerAccess(ReadWriteAdd::read, h1);
    registerAccess(ReadWriteAdd::write, h2);
    ostringstream os;
    os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_syrk " ;
    log_name = os.str();
  }
  void run() {
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
#ifdef BLAS
      // void dsyrk(char uplo, char transa, int n, int k, double alpha, 
      //            double *a, int lda, double beta, double *c, int ldc);
      dsyrk('L','N',N,N,-1.0,a,N,1.0,b,N);
#else
      for ( int i = 0 ; i < M; i ++){
	for ( int j =i ; j< M; j++){
	  for ( int k = 0 ; k < M ; k ++) {
	    Mat(b,j,i) -= Mat(a,j,k) *Mat(a,i,k);
	  }
	}	
      }
#endif
      dumpData(b,M,N,'S');
      if (!check_syrk(b,M,N,key) ){
	printf("CalcError in Task:%s,%s\n",dt_task->getName().c_str(),log_name.c_str());
      }
      if (DEBUG_DLB_DEEP) printf("[%ld] : ----------------------------------------\n",pthread_self());
    }
  string get_name(){ return log_name;}
};
/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/
struct GemmTask : public Task<Options, 4> {
  IDuctteipTask *dt_task ;
  bool b_trans,c_decrease;
  string log_name;
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
    void run() {
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
#ifdef BLAS
      // void dgemm(char transa, char transb, int m, int n, int k, 
      //            double alpha, double *a, int lda, double *b, int ldb, double beta, double *c, int ldc);
      char transb = (b_trans)?'T':'N';
      double alpha = (c_decrease)?-1.0:1.0;
      double beta = 1.0;
      dgemm('N',transb,N,N,N,alpha,a,N,b,N,beta,c,N);
#else
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
#endif
      dumpData(c,M,N,'G');
      if (! check_gemm(c,M,N,key) ) {
	printf("CalcError in Task:%s,%s\n",dt_task->getName().c_str(),log_name.c_str());
      }
      if (DEBUG_DLB_DEEP) printf("[%ld] : ----------------------------------------\n",pthread_self());
    }
  string get_name(){ return log_name;}
};
/*----------------------------------------------------------------------------*/
class Cholesky: public  Context
{
private:
  int     Nb,p;
  IData  *M;
  Config *cfg;
  enum {
    GEMM_SYRK_TASK_ID  ,
    POTRF_TRSM_TASK_ID ,
    POTRF_TASK_ID      ,
    TRSM_TASK_ID       ,
    SYRK_TASK_ID       ,
    GEMM_TASK_ID       ,
    GEMM_ROW_TASK_ID   ,
    GEMM_COL_TASK_ID
  };
  enum Kernels{
    potrf_Kernel,
    trsm_Kernel,
    gemm_Kernel,
    syrk_Kernel
  };
public:
/*----------------------------------------------------------------------------*/
   Cholesky(Config *cfg_,IData *inData=NULL)  {
    cfg = cfg_ ;
    name=static_cast<string>("chol");
    Nb = cfg->getXBlocks();
      TRACE_LOCATION;
    p = cfg->getP_pxq();
    if ( inData == NULL ) {
      M= new Data ("M",cfg->getXDimension(),cfg->getXDimension(),this);
      M->setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
      M->setLocalNumBlocks(cfg->getXLocalBlocks(),cfg->getXLocalBlocks());
      M->setPartition ( Nb,Nb ) ;      
    }
    else{
      M= inData;
    }
    addInOutData(M);
    populateMatrice();
    dtEngine.dumpTime();
  }
/*----------------------------------------------------------------------------*/
  ~Cholesky(){
    if (M->getParent() == this) {
      delete M;
    }
  }
/*----------------------------------------------------------------------------*/
  void dumpAllData(){
    int I = cfg->getYBlocks();        // Mb
    int J = cfg->getXBlocks();        // Nb
    int R = cfg->getYDimension() / I; // #rows per block== M/ Mb
    int C = cfg->getXDimension() / J; // #cols per block== N/ Nb
    int mb = cfg->getYLocalBlocks();
    int nb = cfg->getXLocalBlocks();
    int r = R / mb;
    int c=  C / nb;
    if ( R>20)
      return;
    IData &A=*M;
    for ( int j=0 ; j < J ; j ++){
      for ( int i=j; i < I; i ++ ) {
	if ( A(i,j)->getHost() == me ) {
	  double *contents=A(i,j)->getContentAddress();
	  printf(">>>%s,IJRCMmnrc: %d %d %d %d %d %d %d %d\n",
		 A(i,j)->getName().c_str(),I,J,R,C,mb,nb,r,c);
	  for ( int k=0;k<mb;k++){
	    for (int ii=0;ii<r;ii++){
	      for(int l=0;l<nb;l++){
		for (int jj=0;jj<c;jj++){
		  printf(" %3.0lf ",contents[k*r*c+l*mb*r*c+ii+jj*r]);
		}
	      }
	      // dumpData(contents+k*r*c+l*mb*r*c,r,c,'z');
	      printf("\n");
	    }
	  }
	}
      }
    }
  }
/*----------------------------------------------------------------------------*/

  void populateMatrice(){
    if ( simulation) return;
    int I = cfg->getYBlocks();        // Mb
    int J = cfg->getXBlocks();        // Nb
    int K = cfg->getYDimension() / I; // #rows per block== M/ Mb
    int L = cfg->getXDimension() / J; // #cols per block== N/ Nb
    IData &A=*M;
    dt_log.addEventStart(M,DuctteipLog::Populated);
    for ( int i=0; i < I; i ++ ) {
      for ( int j=0 ; j < J ; j ++){
	if ( A(i,j)->getHost() == me ) {
	  for ( int k=0; k < K; k++){
	    for ( int l=0;l <L; l++){
	      int c = j * L + l+1;
	      int r = i * K + k+1;
	      if ( (r ) == (c ) ) 
		A(i,j)->setElement(k,l,r);
	      else{
		double v = ((r) < (c) ? (r) :(c) ) - 2;
		A(i,j)->setElement(k,l,v);
	      }
	      //printf("[%d,%d]A(%d,%d).(%d,%d):%lf\n",r+k,c+l,i,j,k,l,A(i,j)->getElement(k,l));
	    }
	  }
	  A(i,j)->dump();
	}
      }
    }   
    dt_log.addEventEnd(M,DuctteipLog::Populated);
    dumpAllData();
  }
/*----------------------------------------------------------------------------*/
  void checkCorrectness(){
    if ( simulation) return;
    int I = cfg->getYBlocks();        // Mb
    int J = cfg->getXBlocks();        // Nb
    int K = cfg->getYDimension() / I; // #rows per block== M/ Mb
    int L = cfg->getXDimension() / J; // #cols per block== N/ Nb
    IData &A=*M;
    bool found = false;
    for ( int i=0; i < I; i ++ ) {
      for ( int j=0 ; j < J ; j ++){
	if ( A(i,j)->getHost() == me ) {
	  for ( int k=0; k < K; k++){
	    for ( int l=0;l <L; l++){
	      int c = j * L + l+1;
	      int r = i * K + k+1;
	      double exp,v = A(i,j)->getElement(k,l);
	      if ( r  == c  ) exp =  1;
	      else            exp = -1;
	      
	      if ( r>=c && exp != v && !found) {
		printf("error: [%d,%d]A(%d,%d).(%d,%d):%lf != %lf\n",r+k,c+l,i,j,k,l,v,exp);
		found = true;
	      }
	    }
	  }
	}
      }
    }   
  }
/*----------------------------------------------------------------------------*/
  int del_countTasks   (){
    int t=0;
      for ( int i = 0; i< Nb ; i++) {
	  for(int l = 0;l<i;l++)
	      for (int k = i; k<Nb ; k++)
		t++;
	  t++;
	  for( int j=i+1;j<Nb;j++)
	    t++;
      }
      return t;
  }

/*----------------------------------------------------------------------------*/
  void generateTasks(){
    string s = "a";
    char chol_str[5],pnl_str[5],gemm_str[5] ;
    IData &A=*M;
    sprintf(chol_str,"potrf");
    sprintf(pnl_str ,"trsm");
    sprintf(gemm_str,"gemm");


    for ( int i = 0; i< Nb ; i++) {

	  BEGIN_CONTEXT(A.Region(i,Nb-1,0,i-1) , A.RowSlice(i,0,i-1), A.ColSlice(i,i,Nb-1))  // 0.i*2
	  for(int l = 0;l<i;l++){
	    BEGIN_CONTEXT_BY(l,A.ColSlice(l,i,Nb-1) , A.Cell(i,l), A.ColSlice(i,i,Nb-1))       // 0.i*2.L
	      for (int k = i; k<Nb ; k++){
	        AddTask (this,gemm_str,gemm_Kernel, A(k,l) , A(i,l) , A(k,i) ) ;
	      }
	    END_CONTEXT()
	  }
	  END_CONTEXT()

	    AddTask (this, chol_str, potrf_Kernel , A(i,i));

	  BEGIN_CONTEXT( A.Cell(i,i),NULL,A.ColSlice(i,i+1,Nb-1) )                         // 0.i*2+1
	    for( int j=i+1;j<Nb;j++){
	      AddTask(this,pnl_str,trsm_Kernel,A(i,i), A(j,i) ) ;
	    }
	  END_CONTEXT()
    }

  }
/*----------------------------------------------------------------------------*/
  void generateTasksNoContext(){
    string s = "a";
    char chol_str[5],pnl_str[5],gemm_str[5] ,syrk_str[5] ;
    IData &A=*M;
    sprintf(chol_str,"potrf");
    sprintf(pnl_str ,"trsm");
    sprintf(gemm_str,"gemm");
    sprintf(syrk_str,"syrk");

    

    for ( int i = 0; i< Nb ; i++) {
      for(int l = 0;l<i;l++){
	AddTask(this,syrk_str,syrk_Kernel,A(i,l),A(i,i));
	for (int k = i+1; k<Nb ; k++){
	  AddTask (this,gemm_str, gemm_Kernel,A(k,l) , A(i,l) , A(k,i) ) ;
	}
      }
      AddTask (this, chol_str , potrf_Kernel,A(i,i));
      for( int j=i+1;j<Nb;j++){
	AddTask(this,pnl_str,trsm_Kernel,A(i,i), A(j,i) ) ;
      }
    }

  }
/*----------------------------------------------------------------------------*/
  void runKernels(IDuctteipTask *task ){
    switch (task->getKey()){
    case potrf_Kernel:
      PRINT_IF(KERNEL_FLAG)("dt_potrf task starts running.\n");
      potrf_kernel(task);
      break;
    case trsm_Kernel:
      PRINT_IF(KERNEL_FLAG)("dt_trsm task starts running.\n");
      trsm_kernel(task);
      break;
    case gemm_Kernel:
      PRINT_IF(KERNEL_FLAG)("dt_gemm task starts running.\n");
      gemm_kernel(task);
      break;
    case syrk_Kernel:
      PRINT_IF(KERNEL_FLAG)("dt_syrk task starts running.\n");
      syrk_kernel(task);
      break;
    default:
      printf("invalid task key:%ld.\n",task->getKey());
      break;      
    }
  }
/*----------------------------------------------------------------------------*/
  string getTaskName(unsigned long key){
    string s;
    switch(key){
    case potrf_Kernel:
      s.assign("potrf",5);
      break;
    case gemm_Kernel:
      s.assign("gemm",4);
      break;
    case trsm_Kernel:
      s.assign("trsm",4);
      break;
    case syrk_Kernel:
      s.assign("syrk",4);
      break;
    default:
      s.assign("INVLD",5);
      break;
    }
    //printf("task name:%s\n",s.c_str());
    return s;
  }
/*----------------------------------------------------------------------------*/
#define M(a,b) hM[a][b]
#define A(a,b) hA[a][b]
#define B(a,b) hB[a][b]
#define C(a,b) hC[a][b]
#define SG_TASK(t,...) dtEngine.getThrdManager()->submit( new t##Task( __VA_ARGS__ )  )


/*----------------------------------------------------------------------------*/
  void potrf_kernel(IDuctteipTask *task){
    dt_log.addEventStart(task,DuctteipLog::SuperGlueTaskDefine);
    IData *A = task->getDataAccess(0);
    Handle<Options> **hM  =  A->createSuperGlueHandles();
    int nb = cfg->getXLocalBlocks();
    unsigned int count = 0 ; 
    for(int i = 0; i< nb ; i++) {
      for(int l = 0;l<i;l++){
	SG_TASK(Syrk,task,M(i,l),M(i,i));
	count ++;
	for (int k = i+1; k<nb ; k++){
	  SG_TASK (Gemm,task,M(k,l) , M(i,l) , M(k,i) ) ;
	  count ++;
	}
      }
      SG_TASK (Potrf,task,M(i,i));
      count++;
      for( int j=i+1;j<nb;j++){
	SG_TASK(Trsm,task,M(i,i), M(j,i) ) ;
	count++;
      }
    }
    SG_TASK(Sync, task );
    dt_log.addEventEnd(task,DuctteipLog::SuperGlueTaskDefine);   
  }
/*----------------------------------------------------------------------------*/
  void trsm_kernel(IDuctteipTask *task){
    dt_log.addEventStart(task,DuctteipLog::SuperGlueTaskDefine);
    IData *a = task->getDataAccess(0);
    IData *b = task->getDataAccess(1);
    Handle<Options> **hA  =  a->createSuperGlueHandles();  
    Handle<Options> **hB  =  b->createSuperGlueHandles();  
    int nb = cfg->getXLocalBlocks();    
    unsigned int count = 0 ; 
    dtEngine.dumpTime('1');
    for(int i = 0; i< nb ; i++) {
      for(int j = 0;j<nb;j++){
	for(int k= 0;k<i;k++){
	  SG_TASK(Gemm,task,A(i,k),B(j,k),B(j,i));
	  count ++;
	}
	SG_TASK(Trsm,task,A(i,i),B(j,i));
	count ++;
      }

    }
    SG_TASK(Sync,  task );  
    dtEngine.dumpTime();
    dt_log.addEventEnd(task,DuctteipLog::SuperGlueTaskDefine);
  }
/*----------------------------------------------------------------------------*/
  void syrk_kernel(IDuctteipTask *task){
    dt_log.addEventStart(task,DuctteipLog::SuperGlueTaskDefine);
    IData *a = task->getDataAccess(0);
    IData *b = task->getDataAccess(1);
    Handle<Options> **hA  =  a->createSuperGlueHandles();  
    Handle<Options> **hB  =  b->createSuperGlueHandles();  
    //    printf("1\n");
    int nb = cfg->getXLocalBlocks();
    //    printf("2\n");
    unsigned int count = 0 ;
    for(int i = 0; i< nb ; i++) {
      for(int j = 0;j<i+1;j++){
	for(int k = 0;k<nb;k++){
	  if ( i ==j ) 
	    SG_TASK(Syrk,task,A(i,k),B(i,j));
	  else
	    SG_TASK(Gemm,task,A(i,k),A(j,k),B(i,j),true);
	  count ++;
	}
      }
    }
    SG_TASK(Sync, task );  
    dt_log.addEventEnd(task,DuctteipLog::SuperGlueTaskDefine);
  }
/*----------------------------------------------------------------------------*/
  void gemm_kernel(IDuctteipTask *task){
    dt_log.addEventStart(task,DuctteipLog::SuperGlueTaskDefine);
    IData *a = task->getDataAccess(0);
    IData *b = task->getDataAccess(1);
    IData *c = task->getDataAccess(2);
    Handle<Options> **hA  =  a->createSuperGlueHandles();  
    Handle<Options> **hB  =  b->createSuperGlueHandles();  
    Handle<Options> **hC  =  c->createSuperGlueHandles();  
    //    printf("1\n");
    int nb = cfg->getXLocalBlocks();
    //    printf("2\n");
    unsigned int count =0;
    for(int i = 0; i< nb ; i++) {
      for(int j = 0;j<nb;j++){
	for(int k = 0;k<nb;k++){
	  //printf("3 %d,%d,%d\n",i,j,k);
	  SG_TASK(Gemm,task,A(i,k),B(k,j),C(i,j));
	  count ++;
	}
      }
    }
    SG_TASK(Sync, task );  
    dt_log.addEventEnd(task,DuctteipLog::SuperGlueTaskDefine);
  }
/*----------------------------------------------------------------------------*/
#undef C
#undef A
#undef B
};
#endif //__CHOL_FACT_HPP__
