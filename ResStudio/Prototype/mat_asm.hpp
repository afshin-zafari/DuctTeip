#ifndef __MAT_ASM_HPP__
#define __MAT_ASM_HPP__
#include "context.hpp"



class MatrixAssembly : public Context
{
private:
  Context *MA;
  Context *ctxDistance;
  Context *RBF;
  Data *D,*R,*V;
  Config *cfg;
public : 
  enum OutputData{
    Distance,
    RBFresult
  };
  MatrixAssembly (Config *_cfg):
  cfg(_cfg)
  {
    MA= new Context ("MatrixAssembly");
    RBF= new Context ("RBF_Phi");
    ctxDistance= new Context ("Distance");
    PG = new ProcessGrid(cfg->getProcessors(),cfg->getP_pxq(),cfg->getQ_pxq());

    D= new Data ("Dist",cfg->getXDimension(),cfg->getXDimension(),MA);
    R= new Data ("RBF" ,cfg->getXDimension(),cfg->getXDimension(),MA);
    V= new Data ("vP"  ,cfg->getXDimension(),1                   ,MA);
  
    IHostPolicy *hpData = glbCtx.getDataHostPolicy() ;
    printf("%s,%d   %p\n",__FILE__,__LINE__,hpData);
    D->setDataHostPolicy(hpData);
    R->setDataHostPolicy(hpData);
    V->setDataHostPolicy(hpData);


    V->setPartition (cfg->getXBlocks(),1); 
    D->setPartition(cfg->getXBlocks(),cfg->getXBlocks());
    R->setPartition(cfg->getXBlocks(),cfg->getXBlocks());

    MA->setParent(this);
    ctxDistance->setParent (MA);
    RBF->setParent (MA);

    ctxDistance->addInputData (V);
    ctxDistance->addOutputData(D);
    RBF->addInputData  (D);
    RBF->addOutputData (R);

    addOutputData (D) ;
    addOutputData (R) ;
  
  }
  ~MatrixAssembly()
  {
    return;
    delete RBF;
    delete ctxDistance;
    delete MA;
  }
  void generateTasks () {
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
  ProcessGrid *getProcessGrid(){return PG;}
  void resetVersions(){MA->resetVersions();}

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

};
#endif // __MAT_ASM_HPP__
