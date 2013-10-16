#ifndef __MAT_ASM_HPP__
#define __MAT_ASM_HPP__
#include "context.hpp"

#include "superglue.hpp"
#include "option/instr_tasktiming.hpp"

struct DistanceTask : public Task<Options, 3> {
    DistanceTask(Handle<Options> &h1, Handle<Options> &h2, Handle<Options> &h3) {
        registerAccess(ReadWriteAdd::read, &h1);
        registerAccess(ReadWriteAdd::read, &h2);
        registerAccess(ReadWriteAdd::write, &h3);
    }
    void run() {
      int N = getAccess(2).getHandle()->block->Y_ES();
      int M = getAccess(2).getHandle()->block->X_E();
      int size = getAccess(2).getHandle()->block->getMemorySize();
      double *a = getAccess(0).getHandle()->block->getBaseMemory();
      double *b = getAccess(1).getHandle()->block->getBaseMemory();
      double *c = getAccess(2).getHandle()->block->getBaseMemory();
      for ( int i = 0 ; i < M; i ++){
	for ( int j =0 ; j< M; j++){
	  if  ( i*N+j >size){
	    printf("i*N+j <size : %d * %d + %d ? %d\n",i,N,j,size);
	    assert ( i*N + j < size ) ;
	  }
	  c[i*N+j] = ( a[i] -  b[j] ) *2;
	}
      }
      
    }
};




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

    V->setLocalNumBlocks(cfg->getXLocalBlocks(),1                     );
    D->setLocalNumBlocks(cfg->getXLocalBlocks(),cfg->getXLocalBlocks());
    R->setLocalNumBlocks(cfg->getXLocalBlocks(),cfg->getXLocalBlocks());


    V->setPartition(cfg->getXBlocks(), 1                 ); 
    D->setPartition(cfg->getXBlocks(), cfg->getXBlocks() );
    R->setPartition(cfg->getXBlocks(), cfg->getXBlocks() );


     addInputData (V) ;
    addOutputData (D) ;
    addOutputData (R) ;

    populatePoints();
  }
  ~MatrixAssembly()  {
    return;
  }
  void checkCorrectness (){
    return;
    int I = cfg->getYBlocks();        // Mb
    int J = cfg->getXBlocks();        // Nb
    int K = cfg->getYDimension() / I; // #rows per block== M/ Mb
    int L = cfg->getXDimension() / J; // #cols per block== N/ Nb
    IData &Dist=*D;
    for ( int i=0; i < I; i ++ ) {
      for ( int j=0 ; j < J ; j ++){
	if ( Dist(i,j)->getHost() == me ) {
	  for ( int k=0; k < K; k++){
	    for ( int l=0;l <L; l++){
	      double calc_val = Dist(i,j)->getElement(k,l);
	      double vi = i * K + k;
	      double vj = j * K + l;
	      double expected_v = (vi - vj) *2 ;
	      if (expected_v != calc_val ) 
		printf("error : Dist(%d,%d).(%d,%d):%lf,exp:%lf,vi:%lf,vj:%lf\n",
		       i,j,k,l,calc_val,expected_v,vi,vj);
	    }
	  }
	}
      }
    }   
    
  }
  void partitionTest(){
    printf("partition test\n");
    Partition<double> dt(2);
    int N = 12;
    double * mem = new double[N*N];

    for ( int i=0; i < N; i++){
      for(int j = 0 ; j < N ; j++){
	mem[i*N+ j ] = i*N+j;
	printf("(%2d,%2d)=%5.1lf ",i,j,mem[i*N+ j]);
      }
      printf("\n");
    }
    int Nb = 4, nb = 3;
    dt.setBaseMemory (mem , N*N);
    dt.partitionSquare(N,Nb);
    dt.dumpBlocks();
    Partition<double> sg =*dt.getBlock(3,3);
    double *b23_12 = sg.getAddressOfElement(1,2);
    printf("sgMem:%p b23_12Mem:%p\n",sg.getBaseMemory(),b23_12);
    sg.dump();
    delete mem;
    
  }
  void populateMatrices(){
    int I = cfg->getYBlocks();        // Mb
    int J = cfg->getXBlocks();        // Nb
    int K = cfg->getYDimension() / I; // #rows per block== M/ Mb
    int L = cfg->getXDimension() / J; // #cols per block== N/ Nb
    IData &Dist=*V;
    for ( int i=0; i < I; i ++ ) {
      for ( int j=0 ; j < J ; j ++){
	if ( Dist(i,j)->getHost() == me ) {
	  for ( int k=0; k < K; k++){
	    for ( int l=0;l <L; l++){
	      Dist(i,j)->setElement(k,l,k*100+l);
	      printf("Dist(%d,%d).(%d,%d):%lf\n",i,j,k,l,Dist(i,j)->getElement(k,l));
	    }
	  }
	}
      }
    }   
  }
  void populatePoints(){
    printf("pop points\n");
    int n = cfg->getXBlocks();
    int m = cfg->getYDimension() / cfg->getYBlocks();
    IData &P=*V;
    for ( int i=0; i < n; i ++ ) {
      if ( P(i)->getHost() == me ) {
	for ( int j = 0 ; j < m; j ++){
	  P(i)->setElement(j,0,i*m+j);
	  PRINT_IF(0)("P(%d).(%d):%lf\n",i,j,P(i)->getElement(j));
	}
      }
    }
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
      BEGIN_CONTEXT(Dist.All(),NULL,RBF.All() )
    for ( i=0; i< Mb ; i++){
      BEGIN_CONTEXT(Dist.RowSlice(i,0,Nb-1),NULL,RBF.RowSlice(i,0,Nb-1))
	for ( j=0;j<Nb;j++){
	  sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	  AddTask(this,task_name,RBF_Phi_Kernel,Dist(i,j),NULL,RBF(i,j) ) ;
	}
      END_CONTEXT()
    }
    END_CONTEXT()

  }
  void runKernels(IDuctteipTask * task ) {
    switch ( task->getKey() ) {
    case Distance_Kernel:
      PRINT_IF(KERNEL_FLAG)("distance task starts running.\n");
      //distance_kernel(task);
      break;
    case RBF_Phi_Kernel:
      //printf("RBF Phi task starts running.\n");
      break;
    default:
      //printf("invalid task key:%ld.\n",task->getKey());
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
  
  void distance_kernel(IDuctteipTask * task ) {

    IData *v0 = task->getDataAccess(0);
    IData *v1 = task->getDataAccess(1);
    IData *M  = task->getDataAccess(2);

    Handle<Options> **hV0 = v0->createSuperGlueHandles();
    Handle<Options> **hM  =  M->createSuperGlueHandles();
    Handle<Options> **hV1 = v1->createSuperGlueHandles();
    int nb = cfg->getXLocalBlocks();
    for (int  i=0;  i < nb ; i++){
      for (int  j=0; j < nb; ++j){
	dtEngine.getThrdManager()->submit(new DistanceTask(hV0[i][0],hV1[j][0],hM[i][j]) );
      }
    }

    dtEngine.getThrdManager()->wait ( &hM[nb-1][nb-1] );
    
  }
  

};
#endif // __MAT_ASM_HPP__
