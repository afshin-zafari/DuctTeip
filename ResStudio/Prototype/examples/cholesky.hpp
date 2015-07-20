#ifndef __CHOL_FACT_HPP__
#define __CHOL_FACT_HPP__

#include "ductteip.hpp"
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
      if ( !inData->getParent() ){
	setParent(this);
	inData->setDataHandle(createDataHandle());
      }
      M  = inData->clone();
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
    return;
    if ( R>10)
      return;
    IData &A=*M;
    for ( int j=0 ; j < J ; j ++){
      for ( int i=j; i < I; i ++ ) {
	if ( A(i,j)->getHost() == me ) {
	  double *contents=A(i,j)->getContentAddress();
	  for ( int k=0;k<mb;k++){
	    for (int ii=0;ii<r;ii++){
	      for(int l=0;l<nb;l++){
		for (int jj=0;jj<c;jj++){
		  printf(" %3.0lf ",contents[k*r*c+l*mb*r*c+ii+jj*r]);
		}
	      }
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
   if ( !found)
      LOG_INFO(LOG_MULTI_THREAD,"Result is correct.\n");
  }
/*----------------------------------------------------------------------------*/
  void Cholesky_taskified(){
    IData &A=*M;
    int Nb = A.getXNumBlocks();
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
      potrf_kernel(task);
      break;
    case trsm:
      trsm_kernel(task);
      break;
    case gemm:
      gemm_kernel(task);
      break;
    case syrk:
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

#if SUBTASK==1
#define SuperGlue_Submit(dtt,sgt,...) dtt->getTaskExecutor()->subtask(dtt->getKernelTask(), new sgt##Task(dtt, __VA_ARGS__ )  )
#define SG_TASK(dtt,sgt,...) dtt->getTaskExecutor()->subtask(dtt->getKernelTask(), new sgt##Task(dtt, __VA_ARGS__ )  )
#else
#define SG_TASK(t,a,args...) dtEngine.getThrdManager()->submit( new a##Task( t,  ##args )  )
#define SuperGlue_Submit(t,a,args...) dtEngine.getThrdManager()->submit( new a##Task( t, ##args )  )
#endif

/*----------------------------------------------------------------------------*/
  void potrf_kernel(IDuctteipTask *task){
    LOG_EVENT(DuctteipLog::SuperGlueTaskDefine);

    int n;
    DuctTeip_Data  *A = (DuctTeip_Data  *)task->getArgument(0);
    SuperGlue_Data MM(A,n,n);
    for(int i = 0; i< n ; i++) {
      for(int l = 0;l<i;l++){
	SuperGlue_Submit(task,Syrk,MM(i,l),MM(i,i));
	for (int k = i+1; k<n ; k++){
	  SuperGlue_Submit(task,Gemm,MM(k,l) , MM(i,l) , MM(k,i) ) ;
	}
      }
      SuperGlue_Submit (task,Potrf,MM(i,i));
      for( int j=i+1;j<n;j++){
	SuperGlue_Submit(task,Trsm,MM(i,i), MM(j,i) ) ;
      }
    }
#if SUBTASK !=1
    SuperGlue_Submit(task,Sync);
#endif
    LOG_INFO(LOG_MULTI_THREAD,"%s sg-tasks submitted.\n",task->getName().c_str());
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
	  SG_TASK(task,Gemm,A(i,k),B(j,k),B(j,i));
	  count ++;
	}
	SG_TASK(task,Trsm,A(i,i),B(j,i));
	count ++;
      }

    }
#if SUBTASK !=1
    SG_TASK(task,Sync);  
#endif
    dt_log.addEventEnd(task,DuctteipLog::SuperGlueTaskDefine);
    LOG_INFO(LOG_MULTI_THREAD,"%s sg-tasks submitted.\n",task->getName().c_str());
  }
/*----------------------------------------------------------------------------*/
  void syrk_kernel(IDuctteipTask *task){
    dt_log.addEventStart(task,DuctteipLog::SuperGlueTaskDefine);
    IData *a = task->getDataAccess(0);
    IData *b = task->getDataAccess(1);
    Handle<Options> **hA  =  a->createSuperGlueHandles();  
    Handle<Options> **hB  =  b->createSuperGlueHandles();  
    int nb = cfg->getXLocalBlocks();
    unsigned int count = 0 ;
    for(int i = 0; i< nb ; i++) {
      for(int j = 0;j<i+1;j++){
	for(int k = 0;k<nb;k++){
	  if ( i ==j ) 
	    SG_TASK(task,Syrk,A(i,k),B(i,j));
	  else
	    SG_TASK(task,Gemm,A(i,k),A(j,k),B(i,j),true);
	  count ++;
	}
      }
    }
#if SUBTASK !=1
    SG_TASK(task,Sync);  
#endif
    dt_log.addEventEnd(task,DuctteipLog::SuperGlueTaskDefine);
    LOG_INFO(LOG_MULTI_THREAD,"%s sg-tasks submitted.\n",task->getName().c_str());
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
    int nb = cfg->getXLocalBlocks();
    unsigned int count =0;
    for(int i = 0; i< nb ; i++) {
      for(int j = 0;j<nb;j++){
	for(int k = 0;k<nb;k++){
	  SG_TASK(task,Gemm,A(i,k),B(k,j),C(i,j));
	  count ++;
	}
      }    
    }
#if SUBTASK !=1
    SG_TASK(task,Sync);  
#endif
    dt_log.addEventEnd(task,DuctteipLog::SuperGlueTaskDefine);
    LOG_INFO(LOG_MULTI_THREAD,"%s sg-tasks submitted.\n",task->getName().c_str());
  }
/*----------------------------------------------------------------------------*/
#undef C
#undef A
#undef B
};

Cholesky *Cholesky_DuctTeip(DuctTeip_Data &A){

  Cholesky *C=new Cholesky(static_cast<DuctTeip_Data *>(&A));
  C->Cholesky_taskified();
  return C;
}

#endif //__CHOL_FACT_HPP__
