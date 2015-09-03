#include "gtest/gtest.h"
#include "ductteip.hpp"
#include <acml.h>

int g_argc;
char **g_argv;

/*--------------------------------------------------------------------*/
TEST(MLevel,Basic){
  GenData  A,B;
}
/*--------------------------------------------------------------------*/
TEST(MLevel,DefaultParams){
  GenData A(10);
  GenData B(10,20);
  GenData C(10,20,30);
  EXPECT_EQ(A.getElemCountX(),10);
  EXPECT_EQ(B.getElemCountY(),20);
  EXPECT_EQ(C.getElemCountZ(),30);
  EXPECT_EQ(A.getPartCountX(),0);
  EXPECT_EQ(B.getPartCountX(),0);
  EXPECT_EQ(C.getPartCountX(),0);
}
/*--------------------------------------------------------------------*/
TEST(MLevel,Partition1D){
  const int N=12;
  const int K=4;
  GenPartition P(K);
  GenData A(N);
  A.setPartition(&P);
  EXPECT_EQ(A.getPartCountX() ,K);
}
/*--------------------------------------------------------------------*/
TEST(MLevel,Partition2D){
  const int N=12,M=20;
  const int K=4,J=5;
  GenPartition P(K,J);
  GenData A(M,N);
  A.setPartition(&P);
  EXPECT_EQ(A.getPartCountX() ,K);
  EXPECT_EQ(A.getPartCountY() ,J);
  EXPECT_EQ(A.getElemCountX() ,M);
  EXPECT_EQ(A.getElemCountY() ,N);
}
/*--------------------------------------------------------------------*/
TEST(MLevel,Partition3D){
  const int N=12,M=20,O=30;
  const int K=4,J=5,I=6;
  GenPartition P(K,J,I);
  GenData A(M,N,O);
  A.setPartition(&P);
  EXPECT_EQ(A.getPartCountX() ,K);
  EXPECT_EQ(A.getPartCountY() ,J);
  EXPECT_EQ(A.getPartCountZ() ,I);
  EXPECT_EQ(A.getElemCountX() ,M);
  EXPECT_EQ(A.getElemCountY() ,N);
  EXPECT_EQ(A.getElemCountZ() ,O);
}
/*--------------------------------------------------------------------*/
TEST(MLevel,OneLevel1D){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N);
  L0.addChild(&L1);
  A.setLevel(&L0);
  EXPECT_EQ(A.getPartCountX() ,K);
  EXPECT_EQ(A.getElemCountX() ,N);
}
/*--------------------------------------------------------------------*/
TEST(MLevel,OneLevel2D){
  const int N=12,M=20;
  const int K=4,J=5;
  GenPartition P0(1,1),P1(K,J);
  GenData A(M,N);
  GenLevel L0(&P0),L1(&P1);
  L0.addChild(&L1);
  A.setLevel(&L0);
  EXPECT_EQ(A.getPartCountX() ,K);
  EXPECT_EQ(A.getPartCountY() ,J);
  EXPECT_EQ(A.getElemCountX() ,M);
  EXPECT_EQ(A.getElemCountY() ,N);
}
/*--------------------------------------------------------------------*/
TEST(MLevel,OneLevel3D){
  const int N=12,M=20,O=30;
  const int K=4,J=5,I=6;
  GenPartition P0(1,1,1),P1(K,J,I);
  GenData A(M,N,O);
  GenLevel L0(&P0),L1(&P1);
  L0.addChild(&L1);
  A.setLevel(&L0);
  EXPECT_EQ(A.getPartCountX() ,K);
  EXPECT_EQ(A.getPartCountY() ,J);
  EXPECT_EQ(A.getPartCountZ() ,I);
  EXPECT_EQ(A.getElemCountX() ,M);
  EXPECT_EQ(A.getElemCountY() ,N);
  EXPECT_EQ(A.getElemCountZ() ,O);
}
/*--------------------------------------------------------------------*/
TEST(MLevel,OneLevel1D_Indexing){
  const int N=12;
  const int K=4;
  GenPartition P0(1), P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  EXPECT_EQ(A.getPartCountX() ,K);
  EXPECT_EQ(A.getElemCountX() ,N);
  EXPECT_EQ(A(0).getElemCountX() ,N/K);
  for (int i=0;i<K;i++){
    EXPECT_EQ(A(i).getElemSpanX_Start(),i*N/K);  
    EXPECT_EQ(A(i).getElemSpanX_End  (),(i+1)*N/K);    
  }
}
/*--------------------------------------------------------------------*/
TEST(MLevel,OneLevel2D_Indexing){
  const int N=20,M=20;
  const int K=4,J=5;
  GenPartition P0(1,1),P1(K,J);
  GenData A(M,N);
  GenLevel L0(&P0),L1(&P1);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  EXPECT_EQ(A(0,0).getElemCountX() ,M/K);
  EXPECT_EQ(A(0,0).getElemCountY() ,N/J);
  for (int i=0;i<K;i++){
    EXPECT_EQ(A(0,i).getElemSpanX_Start(),i*M/K);  
    EXPECT_EQ(A(0,i).getElemSpanX_End  (),(i+1)*M/K);    
  }
  for (int i=0;i<J;i++){
    EXPECT_EQ(A(i,0).getElemSpanY_Start(),i*N/J);  
    EXPECT_EQ(A(i,0).getElemSpanY_End  (),(i+1)*N/J);    
  }
}

/*--------------------------------------------------------------------*/

void taskRW321(DataArg Z, DataArg Y, DataArg X){
  EXPECT_EQ(Z.axs,3);
  EXPECT_EQ(Y.axs,2);
  EXPECT_EQ(X.axs,1);
}

void taskRW123(DataArg X, DataArg Y, DataArg Z){
  EXPECT_EQ(X.axs,1);
  EXPECT_EQ(Y.axs,2);
  EXPECT_EQ(Z.axs,3);
  taskRW321(X,Y,Z);
}

TEST(Later,OneLevel1D_InOutArgs){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N),C(N);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  C.setLevel(&L0);
  C.createChildren();
  A.axs = In;
  B.axs = Out;
  C.axs = InOut;
}
/*--------------------------------------------------------------------*/
TEST(Later,OneLevel2D_InOutArgs){
  const int N=20,M=20;
  const int K=4,J=5;
  
  GenPartition P0(1,1),P1(K,J);
  GenData A(M,N),B(M,N),C(M,N);
  GenLevel L0(&P0),L1(&P1);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  C.setLevel(&L0);
  C.createChildren();
  A.axs = In;
  B.axs = Out;
  C.axs = InOut;
}
/*--------------------------------------------------------------------*/
TEST(MLevel,TwoLevels1D){
  const int N=20;
  const int K=4,J=5;
  GenPartition P0(1),P1(K),P2(J);
  GenLevel L0(&P0),L1(&P1),L2(&P2);
  GenData A(N);
  L1.addChild(&L2);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();

  EXPECT_EQ(A      .getLevel(),&L0);
  EXPECT_EQ(A(0)   .getLevel(),&L1);
  EXPECT_EQ(A(0)(0).getLevel(),&L2);

  EXPECT_EQ(A(0  ).getPartCountX(),J);
  EXPECT_EQ(A(K-1).getPartCountX(),J);

}
/*--------------------------------------------------------------------*/
TEST(MLevel,TwoLevels2D){
  const int N=20,M=20;
  const int K=4,J=5;
  const int K2=4,J2=5;
  GenPartition P0(1,1),P1(K,J),P2(K2,J2);
  GenData A(M,N);
  GenLevel L0(&P0),L1(&P1),L2(&P2);
  L1.addChild(&L2);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  EXPECT_EQ(A         .getLevel(),&L0);
  EXPECT_EQ(A(0  ,0  ).getLevel(),&L1);
  EXPECT_EQ(A(0  ,0  ).getPartCountX(),K2);
  EXPECT_EQ(A(J-1,K-1).getPartCountX(),K2);
  EXPECT_EQ(A(0  ,0  ).getPartCountY(),J2);
  EXPECT_EQ(A(J-1,K-1).getPartCountY(),J2);
}
/*--------------------------------------------------------------------*/
void sampleFunction(DataArg ,DataArg ){};
void TwoArgs(DataArg ,DataArg ){};
taskify(sampleFunction,In,InOut);
taskify(TwoArgs,In,Out);

TEST(Sugaring,TaskifierMacro){
  GenData  A,B;  
  CALL sampleFunction(A,B);
}
/*--------------------------------------------------------------------*/
TEST(Sugaring,NoOfArgs){
  GenData  A, B;
  CALL TwoArgs(A,B);
}
/*--------------------------------------------------------------------*/
void aKernel(DataArg A){
  EXPECT_EQ(A.axs,InOut );
}
void bKernel(DataArg  A, DataArg  B){
  EXPECT_EQ(A.axs,In );
  EXPECT_EQ(B.axs,Out);
}
void cKernel(DataArg  A, DataArg  B,DataArg  C){
  EXPECT_EQ(A.axs,In );
  EXPECT_EQ(B.axs,Out);
  EXPECT_EQ(C.axs,Out);
}
void *pf1=(void*)aKernel;
void *pf2=(void*)bKernel;
void *pf3=(void*)cKernel;

/*----------------------------*/
TEST(Sugaring,RunKernel){
  Args args;
  GenData  A,B,C;
  A.axs = InOut;
  B.axs = Out;
  C.axs = Out;
  args.addArg(&A);
  mlMngr.submitNextLevelTasks(pf1,&args);
  A.axs = In;
  args.addArg(&B);
  mlMngr.submitNextLevelTasks(pf2,&args);
  args.addArg(&C);
  mlMngr.submitNextLevelTasks(pf3,&args);  
}
/*----------------------------*/
TEST(Sugaring,RunKernelMap){
  Args args;
  GenData  A,B,C;
  DefKernel(aKernel);
  DefKernel(bKernel);
  DefKernel(cKernel);
  A.axs = InOut;
  B.axs = Out;
  C.axs = Out;
  args.addArg(&A);
  void *ptk=GetKernel(aKernel);
  mlMngr.submitNextLevelTasks(ptk,&args);
  A.axs = In;
  args.addArg(&B);
  ptk=GetKernel(bKernel);
  mlMngr.submitNextLevelTasks(ptk,&args);
  args.addArg(&C);
  ptk=GetKernel(cKernel);
  mlMngr.submitNextLevelTasks(ptk,&args);  
}
/*----------------------------*/
TEST(Sugaring,RunTaskObject){
  GenData A,B,C;
  Args args;
  A.axs = InOut;
  B.axs = Out;
  C.axs = Out;
  args.addArg(&A);
  tasks_list.clear();
  tasks_list.push_back(new GenTask("aKernel",&args) );
  args.addArg(&B);
  tasks_list.push_back(new GenTask("bKernel",&args) );
  args.addArg(&C);
  tasks_list.push_back(new GenTask("cKernel",&args) );
  mlMngr.runTask(tasks_list[0]);
  A.axs = In;
  mlMngr.runTask(tasks_list[1]);
  mlMngr.runTask(tasks_list[2]);
  
}
/*--------------------------------------------------------------------*/
void AddVect(DataArg A,DataArg B);
taskify(AddVect,In,Out);
void AddVect(DataArg A,DataArg B){
  for ( int i =0; i < A.getPartCountX(); i++){
    CALL AddVect(A(i),B(i));
  }
}
/*----------------------------*/
TEST(ChainedLevels,OneLevel_1D){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  AddVect(A,B);
  EXPECT_EQ(tasks_list.size(),K);  
  /*
  mlMngr.runTask(tasks_list[0]);
  mlMngr.runTask(tasks_list[1]);  
  mlMngr.runTask(tasks_list[2]);
  mlMngr.runTask(tasks_list[3]);
  */
}
/*----------------------------*/
void AddMatrix(DataArg A,DataArg B,DataArg C);
taskify(AddMatrix,In,In,InOut);
void AddMatrix(DataArg A,DataArg B,DataArg C){
  for ( int j=0; j < A.getPartCountX(); j++)
    for ( int i=0; i < A.getPartCountY(); i++)
      CALL AddMatrix(A(i,j),B(i,j),C(i,j));

}
/*----------------------------*/
TEST(ChainedLevels,OneLevel_2D){
  const int N=20,M=20;
  const int K=4,J=5;
  
  GenPartition P0(1,1),P1(K,J);
  GenLevel L0(&P0),L1(&P1);
  GenData A(M,N),B(M,N),C(M,N);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  C.setLevel(&L0);
  C.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  AddMatrix(A,B,C);
  EXPECT_EQ(tasks_list.size(),K*J);  
  mlMngr.runTask(tasks_list[0]);  
  mlMngr.runTask(tasks_list[1]);  
  mlMngr.runTask(tasks_list[2]);
  mlMngr.runTask(tasks_list[3]);
}
/*----------------------------*/
TEST(ChainedLevels,TwoLevels_1D){
  const int N=40;
  const int K=4,J=5;
  GenPartition P0(1),P1(K),P2(J);
  GenLevel L0(&P0),L1(&P1),L2(&P2);
  GenData A(N),B(N);

  L1.addChild(&L2);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A(0).getPartCountX(),J);
  EXPECT_EQ(A(K-1).getPartCountX(),J);
  AddVect(A,B);
  EXPECT_EQ(tasks_list.size(),K);  
  EXPECT_EQ(tasks_list[0]->args->args.size(),2);
  mlMngr.runTask(tasks_list[0]);  
  EXPECT_EQ(tasks_list.size(),K+J);  
}
/*--------------------------------------------------------------------*/
TEST(DataHandles,OneLevel_1D_SG){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N);
  L0.addChild(&L1);
  L0.setType(GenLevel::SG_TYPE);
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A.getDataType(),GenLevel::SG_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::SG_TYPE);

  AddVect(A,B);
  EXPECT_EQ(tasks_list.size(),K);
  EXPECT_EQ(A(0).getDataHandle(),tasks_list[0]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0).getDataHandle(),tasks_list[0]->args->args[1]->getDataHandle());  
}
/*----------------------------*/
TEST(DataHandles,OneLevel_1D_DT){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N);
  L0.setType(GenLevel::DT_TYPE);
  L0.addChild(&L1);
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A.getDataType(),GenLevel::DT_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::DT_TYPE);
  AddVect(A,B);
  EXPECT_EQ(tasks_list.size(),K);
  EXPECT_EQ(A(0).getDataHandle(),tasks_list[0]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0).getDataHandle(),tasks_list[0]->args->args[1]->getDataHandle());  
}

TEST(DataHandles,OneLevel_2D){}
/*----------------------------*/
TEST(DataHandles,TwoLevels_1D){
  const int N=40;
  const int K=4,J=5;
  GenPartition P0(1),P1(K),P2(J);
  GenLevel L0(&P0),L1(&P1),L2(&P2);
  GenData A(N),B(N);
  L0.setType(GenLevel::DT_TYPE);
  L1.setType(GenLevel::DT_TYPE);
  L2.setType(GenLevel::SG_TYPE);
  L0.addChild(&L1);
  L1.addChild(&L2);
  A.setLevel(&L0);
  B.setLevel(&L0);
  B.createChildren();
  A.createChildren();
  EXPECT_EQ(A.getDataType(),GenLevel::DT_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::DT_TYPE);

  EXPECT_EQ(A(0).getDataType(),GenLevel::DT_TYPE);
  EXPECT_EQ(B(0).getDataType(),GenLevel::DT_TYPE);

  EXPECT_EQ(A(0)(0).getDataType(),GenLevel::SG_TYPE);
  EXPECT_EQ(B(0)(0).getDataType(),GenLevel::SG_TYPE);


  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A(0).getPartCountX(),J);
  EXPECT_EQ(A(K-1).getPartCountX(),J);
  AddVect(A,B);
  EXPECT_EQ(tasks_list.size(),K);  
  EXPECT_EQ(tasks_list[0]->args->args.size(),2);
  EXPECT_EQ(A(0).getDataHandle(),tasks_list[0]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0).getDataHandle(),tasks_list[0]->args->args[1]->getDataHandle());  
  mlMngr.runTask(tasks_list[0]);  
  EXPECT_EQ(tasks_list.size(),K+J);  
  EXPECT_EQ(A(0)(0).getDataHandle(),tasks_list[K]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0)(0).getDataHandle(),tasks_list[K]->args->args[1]->getDataHandle());  
}
TEST(DataHandles,TwoLevels_2D){}
/*--------------------------------------------------------------------*/

TEST(GenScheduler,OneLevel_1D_SG){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N);
  L0.setType(GenLevel::SG_TYPE);
  L1.setType(GenLevel::SG_TYPE);
  L0.addChild(&L1);
  L1.scheduler = new SGWrapper();
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A.getDataType(),GenLevel::SG_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::SG_TYPE);
  AddVect(A,B);
  EXPECT_EQ(tasks_list.size(),K);
  EXPECT_EQ(A(0).getDataHandle(),tasks_list[0]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0).getDataHandle(),tasks_list[0]->args->args[1]->getDataHandle());  

  SG.barrier();
}
TEST(GenScheduler,OneLevel_2D_SG){
  const int N=20,M=20;
  const int K=4,J=5;
  
  GenPartition P0(1,1),P1(K,J);
  GenLevel L0(&P0),L1(&P1);
  GenData A(M,N),B(M,N),C(M,N);
  L0.setType(GenLevel::SG_TYPE);
  L1.setType(GenLevel::SG_TYPE);
  L0.addChild(&L1);
  L1.scheduler = new SGWrapper();

  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  C.setLevel(&L0);
  C.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  AddMatrix(A,B,C);
  EXPECT_EQ(tasks_list.size(),K*J);  

  EXPECT_EQ(A.getPartCountX(),K);

  EXPECT_EQ(A.getDataType(),GenLevel::SG_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::SG_TYPE);

  EXPECT_EQ(A(0,0).getDataHandle(),tasks_list[0]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0,0).getDataHandle(),tasks_list[0]->args->args[1]->getDataHandle());  
  SG.barrier();
}
/*--------------------------------------------------------------------*/
TEST(GenScheduler,DISABLED_OneLevel_1D_DT){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N);

  dtEngine.start(g_argc,g_argv);

  L0.setType(GenLevel::DT_TYPE);
  L1.setType(GenLevel::DT_TYPE);
  L0.addChild(&L1);
  L1.scheduler = new DTWrapper();
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A.getDataType(),GenLevel::DT_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::DT_TYPE);
  AddVect(A,B);
  EXPECT_EQ(tasks_list.size(),K);
  EXPECT_EQ(A(0).getDataHandle(),tasks_list[0]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0).getDataHandle(),tasks_list[0]->args->args[1]->getDataHandle());  
  for ( int i=0;i<A.getPartCountX();i++){
    initData(A(i));
    initData(B(i));
  }
  dtEngine.finalize();
}
/*--------------------------------------------------------------------*/
class BLASWrapper:public IScheduler{
private:
  enum BLAS_KEYS{
    BLAS_AXPY=0,
    BLAS_AXPY_MAT=1,
    BLAS_LAST_KEY
  };
  map<string,int> blas_map;
public:
  void submitTask(GenTask*t){
    LOG_INFO(LOG_MLEVEL,"BLAS:%s\n",t->fname.c_str());
    EXPECT_EQ(t->args->args[0]->getDataType(),GenLevel::BLAS_TYPE);
    mlMngr.runTask(t);
  }
  void runTask(GenTask *t){
    LOG_INFO(LOG_MLEVEL,"%s\n",t->fname.c_str());
    EXPECT_EQ(t->args->args[0]->getDataType(),GenLevel::BLAS_TYPE);    
    dispatchTask(t);
  }
  void finishedTaskGroup(ulong g){}

  void dispatchTask(GenTask *t){
    double *x,*y,*z;
    int n,m;
    int BLAS_key = blas_map[t->fname];
    switch(BLAS_key){
    case BLAS_AXPY:
      // daxpy(int n, double alpha, double *x, int incx, double *y, int incy);
      x = (double*)t->args->args[0]->getMemory();
      y = (double*)t->args->args[1]->getMemory();
      LOG_INFO(LOG_MLEVEL,"%s run by BLAS x:%p, y:%p\n",t->fname.c_str(),x,y);
      n = t->args->args[0]->getElemCountX();
      if ( x == NULL ) break;
      if ( y == NULL ) break;
      LOG_INFO(LOG_MLEVEL,"%s run by BLAS, n:%d\n",t->fname.c_str(),n);
      LOG_INFO(LOG_MLEVEL,"A(0):%lf, A(1):%lf, A(2):%lf\n",x[0],x[1],x[2]);
      LOG_INFO(LOG_MLEVEL,"B(0):%lf, B(1):%lf, B(2):%lf\n",y[0],y[1],y[2]);
      daxpy( n, 1.0, x, 1, y,1);
      break;
    case BLAS_AXPY_MAT:
      x = (double*)t->args->args[0]->getMemory();
      y = (double*)t->args->args[1]->getMemory();
      z = (double*)t->args->args[2]->getMemory();
      LOG_INFO(LOG_MLEVEL,"%s run by BLAS x:%p, y:%p, z:%p\n",t->fname.c_str(),x,y,z);
      n = t->args->args[0]->getElemCountX(); // columns
      m = t->args->args[0]->getElemCountY(); // rows
      if ( x == NULL ) break;
      if ( y == NULL ) break;
      if ( z == NULL ) break;
      LOG_INFO(LOG_MLEVEL,"%s run by BLAS n:%d, m:%d\n",t->fname.c_str(),n,m);
      for (  int c=0;c<n;c++){
	dcopy( m,      y+c*m, 1, z+c*m,1);
	daxpy( m, 1.0, x+c*m, 1, z+c*m,1);
      }
      break;
    }
  }

  BLASWrapper(){
    blas_map["AddVect"]=BLAS_AXPY;
    blas_map["AddMatrix"]=BLAS_AXPY_MAT;
  }
};
TEST(GenScheduler,OneLevel_1D_BLAS){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N);
  L0.setType(GenLevel::BLAS_TYPE);
  L1.setType(GenLevel::BLAS_TYPE);
  L0.addChild(&L1);
  L1.scheduler = new BLASWrapper();
  A.setLevel(&L0);
  A.createChildren();
  B.setLevel(&L0);
  B.createChildren();
  tasks_list.clear();
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A.getDataType(),GenLevel::BLAS_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::BLAS_TYPE);
  AddVect(A,B);
  EXPECT_EQ(tasks_list.size(),K);
  EXPECT_EQ(A(0).getDataHandle(),tasks_list[0]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0).getDataHandle(),tasks_list[0]->args->args[1]->getDataHandle());  
}
/*--------------------------------------------------------------------*/
TEST(DataMemory,One_Level_1D){
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N);
  L0.addChild(&L1);
  L0.setMemoryAllocation(true);
  A.setLevel(&L0);
  A.allocateMemory();
  A.createChildren();
  B.setLevel(&L0);
  B.allocateMemory();
  B.createChildren();
  EXPECT_NE( A(0).getMemory(),(byte *)NULL);
  EXPECT_NE( A(0).getMemory(),A(1).getMemory());
}
/*--------------------------------------------------------------------*/
TEST(DataMemory,One_Level_2D){
  const int N=20,M=20;
  const int K=4,J=5;
  GenPartition P0(1,1),P1(K,J);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N,M),B(N,M);
  L0.addChild(&L1);
  L0.setMemoryAllocation(true);
  A.setLevel(&L0);
  A.allocateMemory();
  A.createChildren();
  B.setLevel(&L0);
  B.allocateMemory();
  B.createChildren();
  EXPECT_NE( A(0).getMemory(),(byte *)NULL);
  EXPECT_NE( A(0).getMemory(),A(1).getMemory());
}
/*--------------------------------------------------------------------*/
TEST(DataMemory,Two_Levels_1D){
  const int N=200;
  const int K=4,J=5;
  GenPartition P0(1),P1(K),P2(J);
  GenLevel L0(&P0),L1(&P1),L2(&P2);
  GenData A(N),B(N);
  L1.addChild(&L2);
  L0.addChild(&L1);
  L0.setMemoryAllocation(true);
  A.setLevel(&L0);
  A.allocateMemory();
  A.createChildren();
  B.setLevel(&L0);
  B.allocateMemory();
  B.createChildren();
  EXPECT_NE( A(0).getMemory(),(byte *)NULL);
  EXPECT_NE( A(0).getMemory(),A(1).getMemory());

  EXPECT_NE( B(0).getMemory(),(byte *)NULL);
  EXPECT_NE( B(0).getMemory(),B(1).getMemory());
  
  EXPECT_EQ( A(0)(0).getMemory()+N/K/J*sizeof(double),A(0)(1).getMemory());
  EXPECT_EQ( A(0)(1).getMemory()+N/K/J*sizeof(double),A(0)(2).getMemory());
  EXPECT_EQ( A(0)(2).getMemory()+N/K/J*sizeof(double),A(0)(3).getMemory());
  
}
/*--------------------------------------------------------------------*/
TEST(DataMemory,Two_Levels_2D){
  const int N=200,M=200;
  const int K=4,J=5;
  const int K2=5,J2=4;
  GenPartition P0(1,1),P1(K,J),P2(K2,J2);
  GenLevel L0(&P0),L1(&P1),L2(&P2);
  GenData A(N,M),B(N,M);
  L1.addChild(&L2);
  L0.addChild(&L1);
  L0.setMemoryAllocation(true);
  A.setLevel(&L0);
  A.allocateMemory();
  A.createChildren();
  B.setLevel(&L0);
  B.allocateMemory();
  B.createChildren();
  EXPECT_NE( A(0,0).getMemory(),(byte *)NULL);
  EXPECT_NE( B(0,0).getMemory(),(byte *)NULL);
  
  EXPECT_EQ( A(0,0).getMemory()                                    ,A(0,0)(0,0).getMemory());
  EXPECT_EQ( A(0,0).getMemory()+  (N/K/K2)*(M/J/J2)*sizeof(double) ,A(0,0)(1,0).getMemory());
  EXPECT_EQ( A(0,0).getMemory()+2*(N/K/K2)*(M/J/J2)*sizeof(double) ,A(0,0)(2,0).getMemory());

  EXPECT_EQ( A(0,0).getMemory()+(1*J2+0)*(N/K/K2)*(M/J/J2)*sizeof(double) ,A(0,0)(0,1).getMemory());
  EXPECT_EQ( A(0,0).getMemory()+(2*J2+1)*(N/K/K2)*(M/J/J2)*sizeof(double) ,A(0,0)(1,2).getMemory());
  tasks_list.clear();
  AddVect(A,B);
  EXPECT_NE( tasks_list[0]->args->args[0]->getMemory(),(byte *)NULL);
}
/*--------------------------------------------------------------------*/
TEST(PredefinedLevels,BLAS){

  LOG_INFO(LOG_MLEVEL,"----------------PredefinedLevels,BLAS---------------\n");
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N),B(N);
  L0.setType(GenLevel::BLAS_TYPE);
  L1.setType(GenLevel::BLAS_TYPE);
  L1.scheduler = new BLASWrapper();
  L0.addChild(&L1);
  L0.setMemoryAllocation(true);
  A.setLevel(&L0);
  A.allocateMemory();
  A.createChildren();
  B.setLevel(&L0);
  B.allocateMemory();
  B.createChildren();
  tasks_list.clear();
  for ( int i=0;i<K;i++){
    for ( int j=0;j<N/K;j++){
      A(i).setElementValue(j,1.0);
      B(i).setElementValue(j,2.0);
    }
  }
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A.getDataType(),GenLevel::BLAS_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::BLAS_TYPE);
  AddVect(A,B);
  EXPECT_NE( tasks_list[0]->args->args[0]->getMemory(),(byte *)NULL);
  EXPECT_EQ(tasks_list.size(),K);
  EXPECT_EQ(A(0).getDataHandle(),tasks_list[0]->args->args[0]->getDataHandle());  
  EXPECT_EQ(B(0).getDataHandle(),tasks_list[0]->args->args[1]->getDataHandle());  
  //for ( int t=0;t<tasks_list.size();t++)    mlMngr.runTask(tasks_list[t]);
  for ( int i=0;i<K;i++){
    for ( int j=0;j<N/K;j++){
      EXPECT_EQ( B(i).getElementValue(j) , 3);
    }
  }
}
/*--------------------------------------------------------------------*/
TEST(PredefinedLevels,BLAS_2D){

  LOG_INFO(LOG_MLEVEL,"----------------PredefinedLevels,BLAS_2D------------\n");
  const int N=20,M=20;
  const int K=4,J=5;
  GenPartition P0(1,1),P1(K,J);
  GenLevel L0(&P0),L1(&P1);
  GenData A(N,M),B(N,M),C(N,M);
  L0.setType(GenLevel::BLAS_TYPE);
  L1.setType(GenLevel::BLAS_TYPE);
  L1.scheduler = new BLASWrapper();
  L0.addChild(&L1);
  L0.setMemoryAllocation(true);

  A.setLevel(&L0);
  A.allocateMemory();
  A.createChildren();

  B.setLevel(&L0);
  B.allocateMemory();
  B.createChildren();

  C.setLevel(&L0);
  C.allocateMemory();
  C.createChildren();
  tasks_list.clear();
  for ( int i=0;i<J;i++){
    for ( int j=0;j<K;j++){
      for ( int r=0;r<M/J;r++){
	for ( int c=0;c<N/K;c++){
	  A(i,j).setElementValue(r,c,1.0);
	  B(i,j).setElementValue(r,c,2.0);
	  C(i,j).setElementValue(r,c,4.0);// will be overwritten; C=A+B
	}
      }
    }
  }
  EXPECT_EQ(A.getPartCountX(),K);
  EXPECT_EQ(A.getDataType(),GenLevel::BLAS_TYPE);
  EXPECT_EQ(B.getDataType(),GenLevel::BLAS_TYPE);
  AddMatrix(A,B,C);
  EXPECT_NE( tasks_list[0]->args->args[0]->getMemory(),(byte *)NULL);
  EXPECT_EQ(tasks_list.size(),K*J);
  //for ( int t=0;t<tasks_list.size();t++)    mlMngr.runTask(tasks_list[t]);
  for ( int i=0;i<J;i++){
    for ( int j=0;j<K;j++){
      for ( int r=0;r<M/J;r++){
	for ( int c=0;c<N/K;c++){
	  EXPECT_EQ( C(i,j).getElementValue(r,c) , 3.0);
	}
      }
    }
  }
}
/*--------------------------------------------------------------------*/

TEST(PredefinedLevels,SG_BLAS_1D){

  LOG_INFO(LOG_MLEVEL,"----------------PredefinedLevels,SG_BLAS------------\n");
  const int N=12;
  const int K=4;
  GenPartition P0(1),P1(K),P2(1);
  GenLevel L0(&P0),L1(&P1),L2(&P2);
  GenData A(N),B(N);
  L0.setType(GenLevel::SG_TYPE);
  L1.setType(GenLevel::SG_TYPE);
  L2.setType(GenLevel::BLAS_TYPE);
  L1.scheduler = new SGWrapper();

  L2.scheduler = new BLASWrapper();

  L0.setMemoryAllocation(true);
  L0.addChild(&L1);
  L1.addChild(&L2);

  A.setLevel(&L0);
  A.allocateMemory();
  A.createChildren();

  B.setLevel(&L0);
  B.allocateMemory();
  B.createChildren();

  tasks_list.clear();

  for ( int i=0;i<K;i++){
    for ( int j=0;j<N/K;j++){
      A(i).setElementValue(j,1.0);
      EXPECT_EQ(A(i).getPartCountX(),1);
      EXPECT_EQ(B(i).getPartCountX(),1);
      B(i).setElementValue(j,2.0);
    }
  }
  EXPECT_EQ(A.getPartCountX(),K);
  AddVect(A,B);

  /* for ( int t=0;t<tasks_list.size();t++){
    if (tasks_list[t]->args->args[0]->getDataType()==GenLevel::BLAS_TYPE){
      LOG_INFO(LOG_MLEVEL,"task[%d] run.\n",t);
      mlMngr.runTask(tasks_list[t]);
    }
  }
  */
  SG.barrier();
  for ( int i=0;i<K;i++){
    for ( int j=0;j<N/K;j++){
      EXPECT_EQ( B(i).getElementValue(j) , 3);
    }
  }


}
TEST(PredefinedLevels,DT_SG_BLAS){
  LOG_INFO(LOG_MLEVEL,"----------------PredefinedLevels,DT_SG_BLAS------------\n");
  const int N=6;
  const int K=2,J=3;
  GenPartition P0(1),P1(K),P2(J),P3(1);
  GenLevel L0(&P0),L1(&P1),L2(&P2),L3(&P3);
  GenData A(N),B(N);

  dtEngine.start(g_argc,g_argv);

  L0.setType(GenLevel::DT_TYPE);
  L1.setType(GenLevel::DT_TYPE);
  L2.setType(GenLevel::SG_TYPE);
  L3.setType(GenLevel::BLAS_TYPE);

  L1.scheduler = new DTWrapper();
  L2.scheduler = new SGWrapper();
  L3.scheduler = new BLASWrapper();

  L0.setMemoryAllocation(true);
  L0.addChild(&L1);
  L1.addChild(&L2);
  L2.addChild(&L3);

  A.setLevel(&L0);
  A.allocateMemory();
  A.createChildren();

  B.setLevel(&L0);
  B.allocateMemory();
  B.createChildren();

  tasks_list.clear();

  for ( int i=0;i<K;i++){
    for ( int j=0;j<J;j++){
      for ( int k=0;k<N/K/J;k++){
	LOG_INFO(LOG_MLEVEL,"A(%d)(%d).(%d)\n",i,j,k);
	A(i)(j).setElementValue(k,1.0);
	B(i)(j).setElementValue(k,2.0);
	EXPECT_NE(A(i)      .getMemory(),(byte*) NULL);
	EXPECT_NE(A(i)(j)   .getMemory(),(byte*) NULL);
	EXPECT_NE(A(i)(j)(k).getMemory(),(byte*) NULL);
      }
    }
  }
  EXPECT_EQ(A.getPartCountX(),K);
  AddVect(A,B);

  for ( int t=0;t<K;t++){
    initData(A(t));
    initData(B(t));
    //    LOG_INFO(LOG_MLEVEL,"task[%d] run.\n",t);
    //    mlMngr.runTask(tasks_list[t]);
  }
  while(tasks_list.size()<5)continue;
  LOG_INFO(LOG_MLEVEL,"total tasks:%d\n",tasks_list.size());
  SG.barrier();
  dtEngine.finalize();
  for ( int i=0;i<K;i++){
    for ( int j=0;j<J;j++){
      if (B(i).getHost() ==me ){
	for ( int k=0;k<N/K/J;k++){
	  EXPECT_EQ( B(i)(j).getElementValue(k) , 3)<<i<<","<<j<<","<<k<<"\n";
	}
      }
    }
  }


}
TEST(PredefinedLevels,SG_CUBLAS){}
/*--------------------------------------------------------------------*/
TEST(LevelTree,ThreeLevels_1D){
  /*
      L0 
    L11 L12 
    L2
         L3
   */
}
TEST(LevelTree,1DT_2SG_CUBLAS_3BLAS){}
TEST(Later,PredefinedLevels_DT_SG_CUBLAS){}
TEST(Later,PredefinedLevels_DT_DT_SG_BLAS){}

TEST(Later,HierarchyConfig){}
TEST(Later,VectorNorm){}
TEST(Later,IterativeAlgorithms){}
TEST(Later,IndirectAccess){}
TEST(Later,MultiGPU){}
TEST(Later,XeonPhi){}
TEST(Later,LUPP){}
/*--------------------------------------------------------------------*/
TEST(Data,Complex){}
TEST(Data,Hetero){}
TEST(Data,CustomPartition){}
TEST(Data,Sparse){}
TEST(Data,Slices){}
/*--------------------------------------------------------------------*/
TEST(Cloud,GetData){}
TEST(Cloud,GetDataStreaming){}
TEST(Cloud,SendData){}
TEST(Cloud,BLASaaS){}
/*--------------------------------------------------------------------*/

int main (int argc , char **argv){
  ::testing::InitGoogleTest(&argc,argv);
  g_argc = argc;
  g_argv = argv;

  int r= RUN_ALL_TESTS();
  return r;
}


