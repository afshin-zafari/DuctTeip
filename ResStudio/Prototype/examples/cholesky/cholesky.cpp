#include "cholesky.hpp"
//typedef Data DuctTeip_Data;
void Cholesky::taskified(){/*@\label{line:taskified_start}@*/
  Data &A= *M;
  int Nb = A.getXNumBlocks();
  //  LOG_INFO(LOG_DATA,
  printf("Memory Type of Cholesky data:%d, host type=%d\n",  A.getMemoryType(),A.getHostType());
  for(int i = 0; i<Nb; i++){
    for(int j = 0; j<i; j++){
      // submit task for $ \color{cmtmgray}A_{ii}=A_{ij}A_{ij}^T$
      addTask(SYRK, A(i,j), A(i,i));
      for(int k = i+1; k<Nb; k++){
	// submit task for $ \color{cmtmgray}A_{ki}=A_{kj}A_{ij}$
	addTask(GEMM, A(k,j), A(i,j), A(k,i));
      }
    }
    // submit task for $ \color{cmtmgray}A_{ii}\rightarrow LL^T$
    addTask(POTRF, A(i,i));
    for(int j = i+1; j<Nb; j++){
      // submit task for $ \color{cmtmgray}A_{ji}=A_{ii}^{-1}A_{ji}$
      addTask(TRSM, A(i,i), A(j,i));
    }
  }
}/*@\label{line:taskified_end}@*/
//=======================================================
void Cholesky::runKernels(DuctTeip_Task *task ) /*@\label{line:runkernelsdef_start}@*/
{
  switch (task->getKey()){
    case POTRF:   POTRF_kernel(task);      break;/*@\label{line:switchpotrf}@*/
    case  TRSM:    TRSM_kernel(task);      break;
    case  GEMM:    GEMM_kernel(task);      break;
    case  SYRK:    SYRK_kernel(task);      break;
    default:
      fprintf(stderr,"invalid task key:%ld.\n",
	      task->getKey());
      exit(1);
      break;
    }
}/*@\label{line:runkernelsdef_end}@*/
//=======================================================
void Cholesky::POTRF_kernel(DuctTeip_Task *dt_task){/*@\label{line:pkdef_start}@*/
  // Get the argument of the POTRF task
  DuctTeip_Data  *A = dt_task->getArgument(0);/*@\label{line:pkdtdata}@*/
  assert(A);

  // Retrieve the corresponding SuperGlue blocks 
  //   from the DuctTeip data
  assert(A->getSuperGlueData());
  SuperGlue_Data &M = *A->getSuperGlueData();/*@\label{line:pksgdata}@*/
  LOG_INFO(LOG_DATA,"\n");
  int n = M.get_rows_count();

  // Blocks of A can be accessed using '(i,j)' indexing and 
  //   passd as the SuperGlue tasks' arguments
  for(int i = 0; i<n ; i++){
   for(int j = 0; j<i ; j++){
    // create and submit task for $ \color{cmtmgray}M_{ii}=M_{ij}M_{ij}^T$      
     LOG_INFO(LOG_DATA,"\n");
    SyrkTask *syrk = new SyrkTask(dt_task, M(i,j), M(i,i)); /*@\label{line:pksyrk}@*/
    dt_task->subtask(syrk);
     LOG_INFO(LOG_DATA,"\n");
    for (int k = i+1; k<n ; k++){
     // create and submit task for $ \color{cmtmgray}M_{ki}=M_{kj}M_{ij}$
     GemmTask *gemm=new GemmTask(dt_task,M(k,j),M(i,j),M(k,i));/*@\label{line:pkgemm}@*/
     dt_task->subtask(gemm);
    }
   }
   // submit task for $ \color{cmtmgray}M_{ii}\rightarrow LL^T$
   PotrfTask *potrf = new PotrfTask(dt_task, M(i,i));/*@\label{line:pkpotrf}@*/
   dt_task->subtask(potrf);
     LOG_INFO(LOG_DATA,"\n");
   for( int j = i+1; j<n ; j++){
     // submit task for $ \color{cmtmgray}M_{ji}=M_{ii}^{-1}M_{ji}$
     TrsmTask *trsm = new TrsmTask(dt_task, M(i,i), M(j,i));/*@\label{line:pktrsm}@*/
     dt_task->subtask(trsm);
   }
     LOG_INFO(LOG_DATA,"\n");
  }
}/*@\label{line:pkdef_end}@*/


/*----------------------------------------------------------------------------*/
void Cholesky::TRSM_kernel(DuctTeip_Task *dt_task)
{
      
  DuctTeip_Data  *a = (DuctTeip_Data *)dt_task->getArgument(0);
  DuctTeip_Data  *b = (DuctTeip_Data *)dt_task->getArgument(1);
  assert(a);
  assert(b);
  
  assert(a->getSuperGlueData());
  assert(b->getSuperGlueData());
  SuperGlue_Data &A = *a->getSuperGlueData();
  SuperGlue_Data &B = *b->getSuperGlueData();
  int n = A.get_rows_count();
  for(int i = 0; i< n ; i++){
    for(int j = 0; j<n; j++){
      for(int k= 0; k<i; k++){
	GemmTask *gemm=new GemmTask(dt_task,A(i,k),B(j,k),B(j,i));
	dt_task->subtask(gemm);
      }
      TrsmTask *trsm = new TrsmTask(dt_task,A(i,i),B(j,i));
      dt_task->subtask(trsm);
    }

  }
}
/*----------------------------------------------------------------------------*/
void Cholesky::SYRK_kernel(DuctTeip_Task *dt_task)
{
  DuctTeip_Data  *a = (DuctTeip_Data *)dt_task->getArgument(0);
  DuctTeip_Data  *b = (DuctTeip_Data *)dt_task->getArgument(1);
  assert(a);
  assert(b);
  assert(a->getSuperGlueData());
  assert(b->getSuperGlueData());

  SuperGlue_Data &A = *a->getSuperGlueData();
  SuperGlue_Data &B = *b->getSuperGlueData();
  int n = A.get_rows_count();
  for(int i = 0; i<n ; i++){
    for(int j = 0; j<i+1; j++){
      for(int k = 0; k<n; k++){
	if ( i ==j ){
	  SyrkTask *syrk = new SyrkTask(dt_task,A(i,k),B(i,j));
	  dt_task->subtask(syrk);
	}
	else{
	  GemmTask *gemm=new GemmTask(dt_task,A(i,k),A(j,k),B(i,j),true);
	  dt_task->subtask(gemm);
	}
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
void Cholesky::GEMM_kernel(DuctTeip_Task  *dt_task)
{
  DuctTeip_Data  *a = (DuctTeip_Data *)dt_task->getArgument(0);
  DuctTeip_Data  *b = (DuctTeip_Data *)dt_task->getArgument(1);
  DuctTeip_Data  *c = (DuctTeip_Data *)dt_task->getArgument(2);
  assert(a);
  assert(b);
  assert(c);
  assert(a->getSuperGlueData());
  assert(b->getSuperGlueData());
  assert(c->getSuperGlueData());

  SuperGlue_Data &A = *a->getSuperGlueData();
  SuperGlue_Data &B = *b->getSuperGlueData();
  SuperGlue_Data &C = *c->getSuperGlueData();
  int n = A.get_rows_count();
  for(int i = 0; i<n; i++){
    for(int j = 0; j<n; j++){
      for(int k = 0; k<n; k++){
	GemmTask *gemm=new GemmTask(dt_task,A(i,k),B(k,j),C(i,j));
	dt_task->subtask(gemm);
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
Cholesky::Cholesky()
{
  name.assign("chol");
  M = new Data("A",config.N,config.N,this);
  Data &A= *M;
  printf("Memory Type of Cholesky data:%d, host type=%d\n",  A.getMemoryType(),A.getHostType());
  if ( M->getParent())
    M->setDataHandle( M->getParent()->createDataHandle(M));
  M->setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
  M->setLocalNumBlocks(config.nb,config.nb);

  LOG_INFO(LOG_DATA,"config.Nb=%d.\n",config.Nb);
  M->setPartition(config.Nb,config.Nb ) ;
  LOG_INFO(LOG_DATA,"cholesky ctor finished.\n");
  //populateMatrice();
  addInOutData(M);
}
/*----------------------------------------------------------------------------*/
string Cholesky::getTaskName(unsigned long key)
{
  string s;
  switch(key)    {
    case POTRF:      s.assign("potrf",5);      break;
    case  GEMM:      s.assign("gemm" ,4);      break;
    case  TRSM:      s.assign("trsm" ,4);      break;
    case  SYRK:      s.assign("syrk" ,4);      break;
    default:         s.assign("INVLD",5);      break;
  }
  return s;
}
/*----------------------------------------------------------------------------*/
void Cholesky::taskFinished(IDuctteipTask *task, TimeUnit dur)
{
  long key = task->getKey();
  double  n = config.N / config.Nb,gflops;
  switch(key) {
    case POTRF:      gflops=((n*n*n/3.0)/dur);      break;
    case  TRSM:      gflops=((n*n*n/6.0)/dur);      break;
    case  SYRK:      gflops=((n*n*n/6.0)/dur);      break;
    case  GEMM:      gflops=(2*(n*n*n/3.0)/dur);    break;
    }
}
/*----------------------------------------------------------------------------*/
void Cholesky::checkCorrectness()
{
  if ( config.simulation) return;
  int mI = cfg->getYBlocks();        // Mb
  int J = cfg->getXBlocks();        // Nb
  int K = cfg->getYDimension() / mI; // #rows per block== M/ Mb
  int L = cfg->getXDimension() / J; // #cols per block== N/ Nb
  IData &A=*M;
  bool found = false;
  for ( int i=0; i < mI; i ++ )    {
    for ( int j=0 ; j < J ; j ++)	{
      if ( A(i,j)->getHost() == me )	    {
	for ( int k=0; k < K; k++)		{
	  for ( int l=0; l <L; l++)		    {
	    int c = j * L + l+1;
	    int r = i * K + k+1;
	    double exp,v = A(i,j)->getElement(k,l);
	    if ( r  == c  ) exp =  1;
	    else            exp = -1;

	    if ( r>=c && exp != v && !found)			{
	      printf("error: [%d,%d]A(%d,%d).(%d,%d):%lf != %lf\n",r+k,c+l,i,j,k,l,v,exp);
	      found = true;
	    }
	  }
	}
      }
    }
  }
  if ( !found)
    LOG_INFO(LOG_MULTI_THREAD,"Result is correct.\n");
}
/*----------------------------------------------------------------------------*/
void Cholesky::populateMatrice()
{
  if ( config.simulation) return;
  
  cfg=&config;
  int mI = cfg->getYBlocks();        // Mb
  int J = cfg->getXBlocks();        // Nb
  int K = cfg->getYDimension() / mI; // #rows per block== M/ Mb
  int L = cfg->getXDimension() / J; // #cols per block== N/ Nb
  IData &A=*M;

  for ( int i=0; i < mI; i ++ )    {
    for ( int j=0 ; j < J ; j ++)	{
      if ( A(i,j)->getHost() == me )	    {
	for ( int k=0; k < K; k++)		{
	  for ( int l=0; l <L; l++)		    {
	    int c = j * L + l+1;
	    int r = i * K + k+1;
	    if ( (r ) == (c ) )
	      A(i,j)->setElement(k,l,r);
	    else			{
	      double v = ((r) < (c) ? (r) :(c) ) - 2;
	      A(i,j)->setElement(k,l,v);
	    }
	    //printf("[%d,%d]A(%d,%d).(%d,%d):%lf\n",r,c,i,j,k,l,A(i,j)->getElement(k,l));
	  }
	}
	A(i,j)->dump();
      }
    }
  }
  dumpAllData();
}
/*----------------------------------------------------------------------------*/
void Cholesky::dumpAllData()
{
  int mI = cfg->getYBlocks();        // Mb
  int J = cfg->getXBlocks();        // Nb
  int R = cfg->getYDimension() / mI; // #rows per block== M/ Mb
  int C = cfg->getXDimension() / J; // #cols per block== N/ Nb
  int mb = cfg->getYLocalBlocks();
  int nb = cfg->getXLocalBlocks();
  int r = R / mb;
  int c=  C / nb;
  return;
  if ( R>10)
    return;
  Data &A=*M;
  for ( int j=0 ; j < J ; j ++)        {
    for ( int i=j; i < mI; i ++ )            {
      if ( A(i,j)->getHost() == me )                {
	double *contents=A(i,j)->getContentAddress();
	for ( int k=0; k<mb; k++)                    {
	  for (int ii=0; ii<r; ii++)                        {
	    for(int l=0; l<nb; l++)                            {
	      for (int jj=0; jj<c; jj++)                                {
		printf(" %3.0lf ",contents[k*r*c+l*mb*r*c+ii+jj*r]);
	      }
	    }
	    printf("\n");
	  }
	}
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
Cholesky::~Cholesky()
{
  if (M->getParent() == this)    {
    delete M;
  }
}
/*----------------------------------------------------------------------------*/
