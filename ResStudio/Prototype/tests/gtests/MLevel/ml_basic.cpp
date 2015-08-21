#include "gtest/gtest.h"
#include "ductteip.hpp"

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
TEST(GenScheduler,OneLevel_1D_DT){
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
public:
  void submitTask(GenTask*t){
    LOG_INFO(LOG_MLEVEL,"BLAS:%s\n",t->fname.c_str());
    EXPECT_EQ(t->args->args[0]->getDataType(),GenLevel::BLAS_TYPE);
  }
  void runTask(GenTask *t){
    LOG_INFO(LOG_MLEVEL,"%s\n",t->fname.c_str());
    EXPECT_EQ(t->args->args[0]->getDataType(),GenLevel::BLAS_TYPE);    
  }
  void finishedTaskGroup(ulong g){}

  BLASWrapper(){}
};
TEST(GenScheduler,DISABLED_OneLevel_1D_BLAS){
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
TEST(Later,LevelLadder){}
TEST(Later,LevelTree){}
TEST(DataMemory,One_Level_1D){}
TEST(DataMemory,One_Level_2D){}
TEST(DataMemory,Two_Levels_1D){}
TEST(DataMemory,Two_Levels_2D){}
TEST(PredefinedLevels,BLAS){}
TEST(PredefinedLevels,SG_BLAS){}
TEST(PredefinedLevels,DT_SG_BLAS){}
TEST(PredefinedLevels,SG_CUBLAS){}
TEST(Later,PredefinedLevels_DT_SG_CUBLAS){}
TEST(Later,PredefinedLevels_DT_DT_SG_BLAS){}
TEST(Later,TreeLevels_1DT_2SG_CUBLAS_3BLAS){}
TEST(Later,HierarchyConfig){}
TEST(Later,VectorNorm){}

/*--------------------------------------------------------------------*/

int main (int argc , char **argv){
  ::testing::InitGoogleTest(&argc,argv);
  g_argc = argc;
  g_argv = argv;
  return RUN_ALL_TESTS();
}
