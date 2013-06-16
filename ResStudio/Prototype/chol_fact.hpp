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
    M->setDataHostPolicy(glbCtx.getDataHostPolicy() ) ;
    M->setPartition ( Nb,Nb ) ;
    addInputData(M);
    addOutputData(M);

  }
  ~Cholesky(){
    delete M;
  }
  int countTasks(){
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
    sprintf(chol_str,"chol");
    sprintf(pnl_str ,"pnlu");
    sprintf(gemm_str,"gemm");


      for ( int i = 0; i< Nb ; i++) {

	//printf("@RECT CTX\n");
	BEGIN_CONTEXT(A.Region(i,Nb-1,0,i-1) , A.RowSlice(i,0,i-1), A.ColSlice(i,i,Nb-1))  // 0.i*2
	  for(int l = 0;l<i;l++){
	    //printf("@GEMM CTX,%d\n",l);
	    BEGIN_CONTEXT_BY(l,A.ColSlice(l,i,Nb-1) , A.Cell(i,l), A.ColSlice(i,i,Nb-1))       // 0.i*2.L
	      for (int k = i; k<Nb ; k++){
		//printf("gemm %s\n",A(k,i)->getName().c_str());
		AddTask (this,gemm_str, A(k,l) , A(i,l) , A(k,i) ) ;
	      }
	    END_CONTEXT()
	  }
	END_CONTEXT()

	//printf("Chol %s\n",A(i,i)->getName().c_str());
	AddTask (this, chol_str , A(i,i));

	//printf("@PANEL CTX\n");
	BEGIN_CONTEXT( A.Cell(i,i),NULL,A.ColSlice(i,i+1,Nb-1) )                         // 0.i*2+1
	  for( int j=i+1;j<Nb;j++){
	    //printf("pnlu %s\n",A(j,i)->getName().c_str());
	    AddTask(this,pnl_str,A(i,i), A(j,i) ) ;
	  }
	END_CONTEXT()
      }

  }

};
#endif //__CHOL_FACT_HPP__