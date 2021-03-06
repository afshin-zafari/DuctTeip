#include "cholesky.hpp"

int main (int argc, char * argv[])
{
  DuctTeip_Start(argc,argv,true);

  //  Data A(config.N,config.N);

  Cholesky *C=new Cholesky();//static_cast<Data *>(&A));
  C->taskified();


  DuctTeip_Finish();
  if ( me == 0){
    double t=dt_log.getStattime(DuctteipLog::ProgramExecution);
    fprintf(stderr,"[****] Time = %lf, N = %ld , NB= %ld , nb= %ld , p= %ld, q = %ld, gf= %lf\n",
	    t,config.N,config.Nb,config.nb,config.p,config.q,
	    double(config.N)*double(config.N)*double(config.N)/3e9/t);
  }
  stringstream  fn;
  fn << "execution_" << config.P << "_" << config.M <<"_" << me <<".log";
  Trace<Options>::dump(fn.str().c_str());
  //C->checkCorrectness();
}
