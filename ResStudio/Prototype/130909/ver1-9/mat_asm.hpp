#ifndef __MAT_ASM_HPP__
#define __MAT_ASM_HPP__
#include "context.hpp"



class MatrixAssembly : public Context
{
private:
  Context *MA;
  Context *Distance;
  Context *RBF;
  ProcessGrid *PG;
  Data *D,*R,*V;
  Config *cfg;
  DataHostPolicy *dhpMA ;
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
  
    dhpMA = new DataHostPolicy(PG) ;
    MA->setDataHostPolicy(dhpMA);
  }
  void generateTasks (int me) {
    int i,j;
    char task_name[20];
    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	if (PG->isInCol(me, j) ){
	  sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	  AddTask(task_name,(*V)(i),(*V)(j),(*D)(i,j) ) ;
	}
      }
    }
    for ( i=0; i< cfg->getYBlocks() ; i++){
      if (PG->isInRow(me, i) ){
	for ( j=0;j<cfg->getXBlocks();j++){
	  sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	  AddTask(task_name,(*D)(i,j),NULL,(*R)(i,j) ) ;
	}
      }
    }

  }
  void generateTasksByPolicy ( int me , IHostPolicy *policy){
    
    int i,j;
    char task_name[20];
    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	if (policy->isInCol(me, j) ){
	  sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	  AddTask(task_name,(*V)(i),(*V)(j),(*D)(i,j) ) ;
	}
      }
    }
    for ( i=0; i< cfg->getYBlocks() ; i++){
      if (policy->isInRow(me, i) ){
	for ( j=0;j<cfg->getXBlocks();j++){
	  sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	  AddTask(task_name,(*D)(i,j),NULL,(*R)(i,j) ) ;
	}
      }
    }
  }
  void generateTasksByPolicyWithPropagate ( int me , IHostPolicy *policy){
    
    int i,j;
    char task_name[20];
    for ( i=0; i< cfg->getYBlocks() ; i++){
      for ( j=0;j<cfg->getXBlocks();j++){
	// if (policy->isInCol(me, j) ){
	if ( glbCtx.Boundry (this,policy->isInRow(me, i)) ) {
	  sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	  AddTask(task_name,(*V)(i),(*V)(j),(*D)(i,j) ) ;
	}
      }
    }
    for ( i=0; i< cfg->getYBlocks() ; i++){
      // if (policy->isInRow(me, i) ){
      if ( glbCtx.Boundry (this,policy->isInRow(me, i)) ) {
	for ( j=0;j<cfg->getXBlocks();j++){
	  sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	  AddTask(task_name,(*D)(i,j),NULL,(*R)(i,j) ) ;
	}
      }
    }
  }
  ProcessGrid *getProcessGrid(){return PG;}
  void resetVersions(){MA->resetVersions();}
};
#endif // __MAT_ASM_HPP__
