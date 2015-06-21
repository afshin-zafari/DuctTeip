#include "cholesky.hpp"



int main (int argc, char * argv[])
{
  DuctTeip_Start(argc,argv);

  DuctTeip_Data A(config.N,config.N);
  printf("1)Data Chol created,Nb=%d.\n",A.getXNumBlocks());

  Cholesky_DuctTeip(A);

  DuctTeip_Finish();

}
