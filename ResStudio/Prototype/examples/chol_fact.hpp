#ifndef __CHOL_FACT_HPP__
#define __CHOL_FACT_HPP__

#include "ductteip.h"
#include "math.h"

//#define ROW_MAJOR 1
#ifdef ROW_MAJOR
# define Mat(mat,i,j) mat[i*N+j]
#else
# define Mat(mat,i,j) mat[j*N+i]
#endif

#include <acml.h>

#define DEBUG_DLB_DEEP 0

#include "check_comp.h"
#include "dt_util.hpp"
#include "potrf.hpp"
#include "syrk.hpp"
#include "trsm.hpp"
#include "gemm.hpp"
/*----------------------------------------------------------------------------*/
class Cholesky: public  Algorithm
{
private:
  DuctTeip_Data  *M;
  enum KernelKeys{potrf,trsm,gemm,syrk};
public:
/*----------------------------------------------------------------------------*/
  Cholesky(DuctTeip_Data *inData=NULL )  {
    name=static_cast<string>("chol");
    if ( inData !=NULL){
      M  = inData;
      M->setParent(this);
      M->configure();
    }else{
      M= new DuctTeip_Data (config.N,config.N,this);
    }
    populateMatrice();
    addInOutData(M);
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
    cfg=&config;
    printf("1%p\n",cfg);
    int I = cfg->getYBlocks();        // Mb
    int J = cfg->getXBlocks();        // Nb
    int K = cfg->getYDimension() / I; // #rows per block== M/ Mb
    int L = cfg->getXDimension() / J; // #cols per block== N/ Nb
    printf("2\n");
    IData &A=*M;
    printf("1\n");
    printf(">>>IJKL: %d %d %d %d\n",I,J,K,L);

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
	      //printf("[%d,%d]A(%d,%d).(%d,%d):%lf\n",r,c,i,j,k,l,A(i,j)->getElement(k,l));
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
  void Cholesky_taskified(){
    IData &A=*M;
    int Nb = A.getXLocalNumBlocks();
    for ( int i = 0; i< Nb ; i++) {
      for(int l = 0;l<i;l++){
	DuctTeip_Submit(syrk,A(i,l),A(i,i));
	for (int k = i+1; k<Nb ; k++){
	  DuctTeip_Submit(gemm,A(k,l) , A(i,l) , A(k,i) ) ;
	}
      }
      DuctTeip_Submit (potrf,A(i,i));
      for( int j=i+1;j<Nb;j++){
	DuctTeip_Submit(trsm,A(i,i), A(j,i) ) ;
      }
    }
  }
/*----------------------------------------------------------------------------*/
  void runKernels(IDuctteipTask *task ){
    switch (task->getKey()){
    case potrf:
      PRINT_IF(KERNEL_FLAG)("dt_potrf task starts running.\n");
      potrf_kernel(task);
      break;
    case trsm:
      PRINT_IF(KERNEL_FLAG)("dt_trsm task starts running.\n");
      trsm_kernel(task);
      break;
    case gemm:
      PRINT_IF(KERNEL_FLAG)("dt_gemm task starts running.\n");
      gemm_kernel(task);
      break;
    case syrk:
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
    case potrf:
      s.assign("potrf",5);
      break;
    case gemm:
      s.assign("gemm",4);
      break;
    case trsm:
      s.assign("trsm",4);
      break;
    case syrk:
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
#define SuperGlue_Submit(t,...) dtEngine.getThrdManager()->submit( new t##Task( __VA_ARGS__ )  )

/*----------------------------------------------------------------------------*/
  void potrf_kernel(IDuctteipTask *task){
    dt_log.addEventStart(task,DuctteipLog::SuperGlueTaskDefine);

    int n;
    DuctTeip_Data  *A = (DuctTeip_Data  *)task->getArgument(0);
    SuperGlue_Data MM(A,n,n);
    for(int i = 0; i< n ; i++) {
      for(int l = 0;l<i;l++){
	SuperGlue_Submit(Syrk,task,MM(i,l),MM(i,i));
	for (int k = i+1; k<n ; k++){
	  SuperGlue_Submit(Gemm,task,MM(k,l) , MM(i,l) , MM(k,i) ) ;
	}
      }
      SuperGlue_Submit (Potrf2,task,MM(i,i));
      for( int j=i+1;j<n;j++){
	SuperGlue_Submit(Trsm,task,MM(i,i), MM(j,i) ) ;
      }
    }
    SuperGlue_Submit(Sync, task );

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

Cholesky *Cholesky_DuctTeip(DuctTeip_Data &A){

  TRACE_LOCATION;
  Cholesky *C=new Cholesky(static_cast<DuctTeip_Data *>(&A));
  TRACE_LOCATION;
  C->Cholesky_taskified();
  TRACE_LOCATION;
  return C;
}

#endif //__CHOL_FACT_HPP__
