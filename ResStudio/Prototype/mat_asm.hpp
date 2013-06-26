#ifndef __MAT_ASM_HPP__
#define __MAT_ASM_HPP__
#include "context.hpp"



class MatrixAssembly : public IContext
{
private:
  Data *D,*R,*V;
  Config *cfg;
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

    V->setPartition (cfg->getXBlocks(),1); 
    D->setPartition(cfg->getXBlocks(),cfg->getXBlocks());
    R->setPartition(cfg->getXBlocks(),cfg->getXBlocks());

     addInputData (V) ;
    addOutputData (D) ;
    addOutputData (R) ;
  
  }
  ~MatrixAssembly()  {
    return;
  }
  
  ProcessGrid *getProcessGrid(){return PG;}

  void generateTasksSingleNode () {
    int i,j;
    char task_name[20];
    IData &P=*V,&Dist=*D,&RBF=*R;

    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,P(i),P(j),Dist(i,j) ) ;
      }
    }
    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,Dist(i,j),NULL,RBF(i,j) ) ;
      }
    }

  }
  void generateTasks           () {
    int i,j,Mb=cfg->getYBlocks(),Nb=cfg->getXBlocks();
    char task_name[20];
    IData &P = *V,&Dist=*D,&RBF=*R;
    BEGIN_CONTEXT(P.All(),NULL,Dist.All())
    for ( i=0; i< Mb ; i++){
      BEGIN_CONTEXT(P.Cell(i,0),P.ColSlice(i,0,Nb-1),Dist.RowSlice(i,0,Nb-1))
      for ( j=0;j<Nb;j++){
	sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,P(i),P(j),Dist(i,j) ) ;
      }
      END_CONTEXT()
    }
    END_CONTEXT()
      BEGIN_CONTEXT(Dist.All(),NULL,RBF.All() )
    for ( i=0; i< Mb ; i++){
      BEGIN_CONTEXT(Dist.RowSlice(i,0,Nb-1),NULL,RBF.RowSlice(i,0,Nb-1))
	for ( j=0;j<Nb;j++){
	  sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	  AddTask(this,task_name,Dist(i,j),NULL,RBF(i,j) ) ;
	}
      END_CONTEXT()
    }
    END_CONTEXT()

  }

};
#endif // __MAT_ASM_HPP__
