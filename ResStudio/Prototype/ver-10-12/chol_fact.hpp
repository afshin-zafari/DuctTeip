#ifndef __CHOL_FACT_HPP__
#define __CHOL_FACT_HPP__

#include "context.hpp"

class Cholesky: public  Context
{
private:
  int Nb,p;
  IData *M;
  Config *config;
public:
  Cholesky(Config *cfg)  {
    config = cfg ;
    name=static_cast<string>("chol");
    Nb = cfg->getXBlocks();
    p = cfg->getP_pxq();
    M= new Data ("M",cfg->getXDimension(),cfg->getXDimension(),this);
    M->setPartition ( Nb,Nb ) ;
    addInputData(M);
    addOutputData(M);

  }
  ~Cholesky(){
    delete M;
  }
  void generateTasks(){
    string s = "a";
    char chol_str[5],pnl_str[5],gemm_str[5] ;
    IData &A=*M;
    sprintf(chol_str,"chol");
    sprintf(pnl_str ,"pnlu");
    sprintf(gemm_str,"gemm");


      for ( int i = 0; i< Nb ; i++) {
	AddTask ( chol_str , A(i,i));
	BEGIN_CONTEXT( A.Cell(i,i),NULL,A.ColSlice(i,i+1,Nb) )
	  for( int j=0;j<Nb;j++)
	    AddTask(pnl_str,A(i,i), A(j,i) ) ;
	END_CONTEXT()
	BEGIN_CONTEXT(A.Region(i,Nb,0,i) , A.RowSlice(i,0,i), A.ColSlice(i,i,Nb))
	  for (int k = i; k<Nb ; k++){
	    BEGIN_CONTEXT(A.RowSlice(k,0,i) , A.RowSlice(i,0,i), A.Cell(k,i))
	      for(int l = 0;l<i;l++){
		AddTask (gemm_str, A(k,l) , A(i,l) , A(k,i) ) ;
	      }
	    END_CONTEXT()
	  }
	END_CONTEXT()
      }

  }
};
#endif //__CHOL_FACT_HPP__
