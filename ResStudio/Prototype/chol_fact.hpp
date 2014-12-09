#ifndef __CHOL_FACT_HPP__
#define __CHOL_FACT_HPP__

#include "context.hpp"
#include "math.h"
#define registerAccess register_access
#define getAccess get_access
#define getHandle  get_handle


void dumpData(double *d,int M,int N){
  return;
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      //printf (" D(%d,%d)=%3.0lf ",i,j,d[i*N+j]);
      printf ("%3.0lf ",d[i*N+j]);
    }
    printf("\n");
  }
  printf("----------------------------------------\n");
}

struct SyncTask : public Task<Options, -1> {
  IDuctteipTask *dt_task ;
  SyncTask(Handle<Options> &h1,IDuctteipTask *task_):dt_task(task_) {
        registerAccess(ReadWriteAdd::write, h1);
    }
  SyncTask(Handle<Options> **h,int M,int N,IDuctteipTask *task_):dt_task(task_) {
    for(int i =0;i<M;i++)
      for (int j=0;j<N;j++)
        registerAccess(ReadWriteAdd::write, h[i][j]);
    }
    void run() {
      dtEngine.dumpTime();
      PRINT_IF(true || KERNEL_FLAG)("sg_sync task starts running.\n");
      dt_task->setFinished(true);
    }
};

struct SyncColumnTask : public Task<Options, -1> {
  IDuctteipTask *dt_task ;
  SyncColumnTask(Handle<Options> &h1,IDuctteipTask *task_):dt_task(task_) {
        registerAccess(ReadWriteAdd::write, h1);
    }
  SyncColumnTask(Handle<Options> **h,int M,int column,IDuctteipTask *task_):dt_task(task_) {
    for(int i =0;i<M;i++)
        registerAccess(ReadWriteAdd::write, h[i][column]);
    }
    void run() {
      dtEngine.dumpTime();
      PRINT_IF(true || KERNEL_FLAG)("sg_sync task starts running.\n");
      dt_task->setFinished(true);
    }
};

struct PotrfTask : public Task<Options, 1> {
    PotrfTask(Handle<Options> &h1) {
        registerAccess(ReadWriteAdd::write, h1);
    }
    void run() {
      PRINT_IF(KERNEL_FLAG)("sg_potrf task starts running. %s\n",getAccess(0).getHandle()->name);
      int N = getAccess(0).getHandle()->block->Y_ES();
      int M = getAccess(0).getHandle()->block->X_E();
      int size = getAccess(0).getHandle()->block->getMemorySize();
      double *a = getAccess(0).getHandle()->block->getBaseMemory();
      dumpData(a,M,N);
      for ( int i = 0 ; i < M; i ++){
	for ( int k = 0 ; k < i ; k ++) {
	  for ( int j =i ; j< M; j++){
	    a[j*N+i] -= a[j*N+k] * a[i*N+k];
	  }
	}	
	a[i*N+i] = sqrt(a[i*N+i]);
	for ( int j =0 ; j< M; j++){
	  a[j*N+i] /= a[i*N+i];
	}
      }
      dumpData(a,M,N);
    }
};
struct SyrkTask : public Task<Options, 2> {
  SyrkTask(Handle<Options> &h1,Handle<Options> &h2) {
        registerAccess(ReadWriteAdd::read, h1);
        registerAccess(ReadWriteAdd::write, h2);
    }
  void run() {
    PRINT_IF(KERNEL_FLAG)("sg_syrk task starts running.A:%s, B:%s\n",
			  getAccess(0).getHandle()->name,
			  getAccess(1).getHandle()->name);
      int N = getAccess(0).getHandle()->block->Y_ES();
      int M = getAccess(0).getHandle()->block->X_E();
      int size = getAccess(0).getHandle()->block->getMemorySize();
      double *a = getAccess(0).getHandle()->block->getBaseMemory();
      double *b = getAccess(1).getHandle()->block->getBaseMemory();
      dumpData(a,M,N);
      dumpData(b,M,N);
      for ( int i = 0 ; i < M; i ++){
	for ( int j =i ; j< M; j++){
	  for ( int k = 0 ; k < M ; k ++) {
	    b[j*N+i] -= a[j*N+k] * a[i*N+k];
	  }
	}	
      }
      dumpData(b,M,N);
    }
};

struct TrsmTask : public Task<Options, 2> {
  TrsmTask(Handle<Options> &h1,Handle<Options> &h2) {
        registerAccess(ReadWriteAdd::read, h1);
        registerAccess(ReadWriteAdd::write, h2);
    }
    void run() {
      PRINT_IF(KERNEL_FLAG)("sg_trsm task starts running.A:%s, B:%s\n",
			  getAccess(0).getHandle()->name,
			  getAccess(1).getHandle()->name);
      int N = getAccess(0).getHandle()->block->Y_ES();
      int M = getAccess(0).getHandle()->block->X_E();
      int size = getAccess(0).getHandle()->block->getMemorySize();
      double *a = getAccess(0).getHandle()->block->getBaseMemory();
      double *b = getAccess(1).getHandle()->block->getBaseMemory();
      for ( int i = 0 ; i < M; i ++){
	for ( int j =0 ; j< M; j++){
	  for ( int k=0;k<i;k++) {
	    b[j*N+i] -=  b[j*N+k] *  a[i*N+k] ;
	  }
	  b[j*N+i] /= a[i*N+i];
	}
      }
    }
};

struct GemmTask : public Task<Options, 3> {
  bool b_trans,c_decrease;
  GemmTask(Handle<Options> &h1,
	   Handle<Options> &h2,
	   Handle<Options> &h3,
	   bool trans_b=false,
	   bool decrease_c = true):
    b_trans(trans_b),
    c_decrease(decrease_c){
        registerAccess(ReadWriteAdd::read, h1);
        registerAccess(ReadWriteAdd::read, h2);
        registerAccess(ReadWriteAdd::write, h3);
    }
    void run() {
      PRINT_IF(KERNEL_FLAG)("sg_gemm task starts running.A:%s,B:%s,C:%s\n",
			    getAccess(0).getHandle()->name,
			    getAccess(1).getHandle()->name,
			    getAccess(2).getHandle()->name );
      int N = getAccess(0).getHandle()->block->Y_ES();
      int M = getAccess(0).getHandle()->block->X_E();
      int size = getAccess(0).getHandle()->block->getMemorySize();
      double *a = getAccess(0).getHandle()->block->getBaseMemory();
      double *b = getAccess(1).getHandle()->block->getBaseMemory();
      double *c = getAccess(2).getHandle()->block->getBaseMemory();
      dumpData(a,M,N);
      dumpData(b,M,N);
      dumpData(c,M,N);
      for ( int i = 0 ; i < M; i ++){
	for ( int j =0 ; j< M; j++){
	  for ( int k =0 ; k< M; k++){
	    if ( c_decrease){
	      if ( b_trans)
		c[i*N+j] -=  a[i*N+k] *  b[j*N+k] ;
	      else
		c[i*N+j] -=  a[i*N+k] *  b[k*N+j] ;
	    }
	    else{
	      if ( b_trans)
		c[i*N+j] +=  a[i*N+k] *  b[j*N+k] ;
	      else
		c[i*N+j] +=  a[i*N+k] *  b[k*N+j] ;
	    }
	  }
	}
      }
      dumpData(c,M,N);

      
    }
};


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
  ~Cholesky(){
    if (M->getParent() == this) 
      delete M;
  }
  void populateMatrice(){
    int I = cfg->getYBlocks();        // Mb
    int J = cfg->getXBlocks();        // Nb
    int K = cfg->getYDimension() / I; // #rows per block== M/ Mb
    int L = cfg->getXDimension() / J; // #cols per block== N/ Nb
    IData &A=*M;
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
	}
      }
    }   
  }
  void checkCorrectness(){
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
  int  countTasks   (){
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
  void generateTasksNoPropagate(){
    string s = "a";
    char chol_str[5],pnl_str[5],gemm_str[5],syrk_str[5] ;
    IData &A=*M;
    sprintf(chol_str,"potrf");
    sprintf(pnl_str ,"trsm");
    sprintf(gemm_str,"gemm");
    sprintf(syrk_str,"syrk");

#define skipFunc(a,b,c) upgradeVersions(a,b,Nb,M,c)


      for ( int i = 0; i< Nb ; i++) {

	printf("gemm + syrk context check?cntr=-1,f=0,t=%d\n",i);
	BEGIN_CONTEXT_EX(DONT_CARE_COUNTER,skipFunc(i,0,GEMM_SYRK_TASK_ID),A.Region(i,Nb-1,0,i-1) , A.RowSlice(i,0,i-1), A.ColSlice(i,i,Nb-1)) 
	  printf("gemm + syrk context entered.\n");
	  for(int l = 0;l<i;l++){
  	      AddTask(this,syrk_str,syrk_Kernel,A(i,l),A(i,i));
	      printf("gemm context check?ctr=%d,f=%d,t=%d\n",l,i+1,Nb);
	      BEGIN_CONTEXT_EX(l,skipFunc(i,l,GEMM_COL_TASK_ID),A.ColSlice(l,i+1,Nb-1) , A.Cell(i,l), A.ColSlice(i,i,Nb-1))       
		printf("gemm context entered\n");
	        for (int k = i+1; k<Nb ; k++){
		  AddTask (this,gemm_str, gemm_Kernel,A(k,l) , A(i,l) , A(k,i) ) ;
	        }
	      END_CONTEXT_EX()
	  }
	END_CONTEXT_EX()


	  printf("potrf+trsm context check?ctr=-1,f=0,t=1\n");
	BEGIN_CONTEXT_EX( DONT_CARE_COUNTER,skipFunc(i,0,POTRF_TRSM_TASK_ID),A.Cell(i,i),NULL,A.ColSlice(i,i+1,Nb-1) )   
	  printf("potrf+trsm context entered.\n");
	  AddTask (this, chol_str , potrf_Kernel,A(i,i));
	  BEGIN_CONTEXT_EX( DONT_CARE_COUNTER,skipFunc(i,0,TRSM_TASK_ID),A.Cell(i,i),NULL,A.ColSlice(i,i+1,Nb-1) )   
	    for( int j=i+1;j<Nb;j++){
	      AddTask(this,pnl_str,trsm_Kernel,A(i,i), A(j,i) ) ;
	    }
	  END_CONTEXT_EX()
	END_CONTEXT_EX()
      }

  }
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
  void upgradeVersions ( int col,int index,int N,IData *M,int task){
    IData &A = *M;
    switch ( task ) {
    case POTRF_TASK_ID:
      A(col,col)->addToVersion(IData::WRITE,1);
      break;
    case TRSM_TASK_ID:
      for ( int i=col+1;i < N ; i++ ) {
	A(i,col)->addToVersion(IData::WRITE,1);
      }
      A(col,col)->addToVersion(IData::READ,N-col-1);      
      break;
    case SYRK_TASK_ID:
      for ( int i=0;i < col ; i++ ) {
	A(col,i)->addToVersion(IData::READ,1);
      }
      A(col,col)->addToVersion(IData::WRITE,col);
      break;
    case GEMM_TASK_ID:
      for ( int c=0;c < col ; c++ ) {
	A(col,c)->addToVersion(IData::READ,N-col-1);
      }
      for ( int r=col+1;r < N ; r++ ) {
	for ( int c=0;c < col ; c++ ) {
	  A(r,c)->addToVersion(IData::READ,1);
	}
      }
      for ( int r=col+1;r < N ; r++ ) {
	A(r,col)->addToVersion(IData::WRITE,col);
      }
      break;
    case GEMM_ROW_TASK_ID:
      for ( int c=0;c < col ; c++ ) {
	A(col  ,c)->addToVersion(IData::READ,1);
	A(index,c)->addToVersion(IData::READ,1);
      }
      A(index,col)->addToVersion(IData::WRITE,col);
      break;
    case GEMM_COL_TASK_ID:
      for ( int r=col+1;r < N ; r++ ) {
	A(r,index)->addToVersion(IData::READ,1);
	A(r,col)->addToVersion(IData::WRITE,1);
      }
      A(col,index)->addToVersion(IData::READ,N-col-1);      
      break;
    case GEMM_SYRK_TASK_ID:
      upgradeVersions(col,0,N,M,SYRK_TASK_ID);
      upgradeVersions(col,0,N,M,GEMM_TASK_ID);
      break;
    case POTRF_TRSM_TASK_ID:
      upgradeVersions(col,0,N,M,POTRF_TASK_ID);
      upgradeVersions(col,0,N,M,TRSM_TASK_ID);
      break;
    }
  }
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
    printf("task name:%s\n",s.c_str());
    return s;
  }
#define M(a,b) hM[a][b]
#define A(a,b) hA[a][b]
#define B(a,b) hB[a][b]
#define C(a,b) hC[a][b]
#define SG_TASK(t,...) dtEngine.getThrdManager()->submit( new t##Task( __VA_ARGS__ )  )


  void potrf_kernel(IDuctteipTask *task){
    IData *A = task->getDataAccess(0);
    Handle<Options> **hM  =  A->createSuperGlueHandles();
    int nb = cfg->getXLocalBlocks();
    for ( int i = 0; i< nb ; i++) {
      for(int l = 0;l<i;l++){
	SG_TASK(Syrk,M(i,l),M(i,i));
	for (int k = i+1; k<nb ; k++){
	  SG_TASK (Gemm,M(k,l) , M(i,l) , M(k,i) ) ;
	}
      }
      SG_TASK (Potrf,M(i,i));
      for( int j=i+1;j<nb;j++){
	SG_TASK(Trsm,M(i,i), M(j,i) ) ;
      }
    }
    SG_TASK(Sync, M(nb-1,nb-1), task );
    
  }
  void trsm_kernel(IDuctteipTask *task){
    IData *a = task->getDataAccess(0);
    IData *b = task->getDataAccess(1);
    Handle<Options> **hA  =  a->createSuperGlueHandles();  
    Handle<Options> **hB  =  b->createSuperGlueHandles();  
    int nb = cfg->getXLocalBlocks();    
    dtEngine.dumpTime('1');
    for(int i = 0; i< nb ; i++) {
      for(int j = 0;j<nb;j++){
	for(int k= 0;k<i;k++){
	  SG_TASK(Gemm,A(i,k),B(j,k),B(j,i));
	}
	SG_TASK(Trsm,A(i,i),B(j,i));
      }

    }
    SG_TASK(SyncColumn, hB,nb,nb-1, task );  
    dtEngine.dumpTime();
  }
  void syrk_kernel(IDuctteipTask *task){
    IData *a = task->getDataAccess(0);
    IData *b = task->getDataAccess(1);
    Handle<Options> **hA  =  a->createSuperGlueHandles();  
    Handle<Options> **hB  =  b->createSuperGlueHandles();  
    int nb = cfg->getXLocalBlocks();
    for(int i = 0; i< nb ; i++) {
      for(int j = 0;j<nb;j++){
	SG_TASK(Gemm,A(i,j),A(i,j),B(i,j),true);
      }
    }
    SG_TASK(Sync, hB,nb,nb, task );  
  }
  void gemm_kernel(IDuctteipTask *task){
    IData *a = task->getDataAccess(0);
    IData *b = task->getDataAccess(1);
    IData *c = task->getDataAccess(2);
    Handle<Options> **hA  =  a->createSuperGlueHandles();  
    Handle<Options> **hB  =  b->createSuperGlueHandles();  
    Handle<Options> **hC  =  c->createSuperGlueHandles();  
    int nb = cfg->getXLocalBlocks();
    for(int i = 0; i< nb ; i++) {
      for(int j = 0;j<nb;j++){
	for(int k = 0;k<nb;k++){
	  SG_TASK(Gemm,A(i,k),B(k,j),C(i,j));
	}
      }
    }
    SG_TASK(Sync, hC,nb,nb, task );  
  }
#undef C
#undef A
#undef B
};
#endif //__CHOL_FACT_HPP__
