#ifndef __MAT_ASM_HPP__
#define __MAT_ASM_HPP__
#include "context.hpp"



class MatrixAssembly : public Context
{
private:
  Context *MA;
  Context *Distance;
  Context *RBF;
  Data *D,*R,*V;
  Config *cfg;
public : 
  MatrixAssembly (Config *_cfg):
  cfg(_cfg)
  {
    MA= new Context ("MatrixAssembly");
    RBF= new Context ("RBF_Phi");
    Distance= new Context ("Distance");
    PG = new ProcessGrid(cfg->getProcessors(),cfg->getP_pxq(),cfg->getQ_pxq());

    D= new Data ("Dist",cfg->getXDimension(),cfg->getXDimension(),MA);
    R= new Data ("RBF" ,cfg->getXDimension(),cfg->getXDimension(),MA);
    V= new Data ("vP"  ,cfg->getXDimension(),1                   ,MA);
  

    V->setPartition (cfg->getXBlocks(),1); 
    D->setPartition(cfg->getXBlocks(),cfg->getXBlocks());
    R->setPartition(cfg->getXBlocks(),cfg->getXBlocks());

    MA->setParent(this);
    Distance->setParent (MA);
    RBF->setParent (MA);

    Distance->addInputData (V);
    Distance->addOutputData(D);
    RBF->addInputData  (D);
    RBF->addOutputData (R);
  
    DataHostPolicy *hpData = new DataHostPolicy(PG) ;
    D->setDataHostPolicy(hpData);
    R->setDataHostPolicy(hpData);
    V->setDataHostPolicy(hpData);
  }
  ~MatrixAssembly()
  {
    return;
    delete RBF;
    delete Distance;
    delete MA;
  }
  void generateTasks (int me) {
    int i,j,Mb=cfg->getYBlocks(),Nb=cfg->getXBlocks();
    char task_name[20];
    BEGIN_CONTEXT(V->All(),NULL,D->All())
    for ( i=0; i< Mb ; i++){
      BEGIN_CONTEXT(V->Cell(i,0),V->ColSlice(i,0,Nb-1),D->RowSlice(i,0,Nb-1))
      for ( j=0;j<Nb;j++){
	sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,(*V)(i),(*V)(j),(*D)(i,j) ) ;
      }
      END_CONTEXT()
    }
    END_CONTEXT()
    BEGIN_CONTEXT(D->All(),NULL,R->All() )
    for ( i=0; i< Mb ; i++){
      BEGIN_CONTEXT(D->RowSlice(i,0,Nb-1),NULL,R->RowSlice(i,0,Nb-1))
	for ( j=0;j<Nb;j++){
	  sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	  AddTask(this,task_name,(*D)(i,j),NULL,(*R)(i,j) ) ;
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
    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,(*V)(i),(*V)(j),(*D)(i,j) ) ;
      }
    }
    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	AddTask(this,task_name,(*D)(i,j),NULL,(*R)(i,j) ) ;
      }
    }

  }

};
#endif // __MAT_ASM_HPP__
