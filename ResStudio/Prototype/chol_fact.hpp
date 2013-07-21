#ifndef __CHOL_FACT_HPP__
#define __CHOL_FACT_HPP__

#include "context.hpp"

class Cholesky: public  Context
{
private:
  int     Nb,p;
  IData  *M;
  Config *config;
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
   Cholesky(Config *cfg,IData *inData=NULL)  {
    config = cfg ;
    name=static_cast<string>("chol");
    Nb = cfg->getXBlocks();
    p = cfg->getP_pxq();
    if ( inData == NULL ) {
      M= new Data ("M",cfg->getXDimension(),cfg->getXDimension(),this);
      M->setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
      M->setPartition ( Nb,Nb ) ;      
    }
    else{
      M= inData;
    }
    addInOutData(M);
  }
  ~Cholesky(){
    if (M->getParent() == this) 
      delete M;
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
  void runKernels(ITask *task ){
    switch (task->getKey()){
    case potrf_Kernel:
      printf("potrf task starts running.\n");
      break;
    case trsm_Kernel:
      printf("trsm task starts running.\n");
      break;
    case gemm_Kernel:
      printf("gemm task starts running.\n");
      break;
    case syrk_Kernel:
      printf("syrk task starts running.\n");
      break;
    default:
      printf("invalid task key:%ld.\n",task->getKey());
      break;      
    }
    task->setFinished(true);
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
};
#endif //__CHOL_FACT_HPP__
