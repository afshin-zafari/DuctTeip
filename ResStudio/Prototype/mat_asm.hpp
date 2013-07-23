#ifndef __MAT_ASM_HPP__
#define __MAT_ASM_HPP__
#include "context.hpp"



class MatrixAssembly : public IContext
{
private:
  Data *D,*R,*V;
  Config *cfg;
  enum Kernels{
    Distance_Kernel,
    RBF_Phi_Kernel
  };

public : 
  enum OutputData{
    Distance,
    RBFresult
  };
   MatrixAssembly (Config *_cfg):IContext("MatrixAssembly"),  cfg(_cfg)  {
    PG = new ProcessGrid(cfg->getProcessors(),cfg->getP_pxq(),cfg->getQ_pxq());

    D= new Data ("Dist",cfg->getXDimension(),cfg->getXDimension(),this);
    R= new Data ("RBF" ,cfg->getXDimension(),cfg->getXDimension(),this);
    V= new Data ("vP"  ,cfg->getXDimension(),1                   ,this);
  
    IHostPolicy *hpData = glbCtx.getDataHostPolicy() ;
    D->setDataHostPolicy(hpData);
    R->setDataHostPolicy(hpData);
    V->setDataHostPolicy(hpData);

    V->setPartition(cfg->getXBlocks(), 1                 ); 
    D->setPartition(cfg->getXBlocks(), cfg->getXBlocks() );
    R->setPartition(cfg->getXBlocks(), cfg->getXBlocks() );

     addInputData (V) ;
    addOutputData (D) ;
    addOutputData (R) ;
  
  }
  ~MatrixAssembly()  {
    return;
  }
  
  ProcessGrid *getProcessGrid(){return PG;}

  void generateTasksNoContext () {
    int i,j;
    char task_name[20];
    IData &P=*V,&Dist=*D,&RBF=*R;

    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,Distance_Kernel,P(i),P(j),Dist(i,j) ) ;
      }
    }
    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,RBF_Phi_Kernel,Dist(i,j),NULL,RBF(i,j) ) ;
      }
    }

  }
  void generateTasks           () {
    int i,j,Mb=cfg->getYBlocks(),Nb=cfg->getXBlocks();
    char task_name[20];
    IData &P = *V,&Dist=*D,&RBF=*R;
    BEGIN_CONTEXT(P.All(),NULL,Dist.All())
    for ( i=0; i< Mb ; i++){
      BEGIN_CONTEXT(P.Cell(i,0),P.ColSlice(0,0,Nb-1),Dist.RowSlice(i,0,Nb-1))
      for ( j=0;j<Nb;j++){
	sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,Distance_Kernel,P(i),P(j),Dist(i,j) ) ;
      }
      END_CONTEXT()
    }
    END_CONTEXT()
      TRACE_LOCATION;
      BEGIN_CONTEXT(Dist.All(),NULL,RBF.All() )
    for ( i=0; i< Mb ; i++){
      BEGIN_CONTEXT(Dist.RowSlice(i,0,Nb-1),NULL,RBF.RowSlice(i,0,Nb-1))
	TRACE_LOCATION;
	for ( j=0;j<Nb;j++){
	  TRACE_LOCATION;
	  sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	  AddTask(this,task_name,RBF_Phi_Kernel,Dist(i,j),NULL,RBF(i,j) ) ;
	}
      END_CONTEXT()
    }
    END_CONTEXT()

  }
  void runKernels(ITask * task ) {
    switch ( task->getKey() ) {
    case Distance_Kernel:
      printf("distance task starts running.\n");
      break;
    case RBF_Phi_Kernel:
      printf("RBF Phi task starts running.\n");
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
    case Distance_Kernel:
      s.assign( "distance",8);
      break;
    case RBF_Phi_Kernel:
      s.assign("RBF_Phi",7);
      break;
    default:
      break;
    }
    printf("task name:%s\n",s.c_str());
    return s;
  }

};
#endif // __MAT_ASM_HPP__
