#include <string>
#include "ductteip.hpp"
#include "fmm_3d.hpp"

using namespace std;
using namespace FMM_3D;

/*---------------------------------------------------------------------------------------*/
void test_data_definition(){
  GData  A(10,10,2);
  A.allocate(10,100);
  byte * m=(byte *)A.getHeaderAddress();
  printf("Content Address %p\n",m);
  std::cout << "ContentAddress :" << (void *)(m) << std::endl;
}
/*---------------------------------------------------------------------------------------*/
void test_data_indexing(){
  GData  A(10,10,2);
  int r =   A(1,1).get_row();
  std::cout << r << std::endl;
}
/*---------------------------------------------------------------------------------------*/
void test_suite_data(){
  test_data_definition();
  test_data_indexing  ();  
}
/*---------------------------------------------------------------------------------------*/
void test_task_definition(){
  GData A(5,4,4);
  A(0,0).setName("A0");
  A(1,1).setName("A1");
  A(2,1).setName("A2");

  fmm_engine->add_task ( new FFLTask(A(0,0),A(1,1),nullptr) );
  fmm_engine->add_task ( new FFLTask(A(1,1),A(2,1),nullptr) );
 }
/*---------------------------------------------------------------------------------------*/
namespace DataCommTestSuite{

  GData *D;
  
  /*----------------------------------------------------------------*/
  void check_results(){
    if ( !D)
      return;
    GData &A=*D;
    GData &a = A(1,1)[1],&b=A(2,2)[2],&c=A(3,3)[3];
    cout <<" Check Results of DataComm\n";
    if ( me ==0 ){
      a.dump();
      a.check_sequence(23.0,2.0);
    }
    if ( me == 1) {
      b.dump();
      b.check_sequence(35.0,3.0);
    }
  }
  /*----------------------------------------------------------------*/
  void run(){
    D = new GData(20,5,4);
    GData &A = *D;
    GData &a = A(1,1)[1],&b=A(2,2)[2],&c=A(3,3)[3];
    
    a.setHost(0);a.setName("a");a.setHostType(IData::SINGLE_HOST); 
    b.setHost(1);b.setName("b");b.setHostType(IData::SINGLE_HOST);
    c.setHost(-1);c.setName("c");
    int N = 5;
    a.allocate(N,N);
    
    if(me==0){
          a.fill_sequence(1.0,1.0);
    }
    b.allocate(N,N);
    if ( me ==1) {
      b.fill_sequence(2.0,1.0);
    }

    c.allocate(N,N);
    c.fill(10.0);

    fmm_engine->add_task(new FFLTask ( c,a,nullptr)); // a= a+ c; MPI_Rank : 0   
    fmm_engine->add_task(new FFLTask ( c,b,nullptr)); // b= b+ c; MPI_Rank : 1   
    fmm_engine->add_task(new FFLTask ( b,a,nullptr));// a= a+ b;  MPI_Rank : 0  
    fmm_engine->add_task(new FFLTask ( a,b,nullptr));// b = b+a;  MPI_Rank : 1  
  }
}

/*---------------------------------------------------------------------------------------*/
namespace TaskTestSuite{
  FFLTask *p;
  void test_task_completion(){
    GData A(5,4,4);
    A(0,0).setName("A0");
    A(1,1).setName("A1");
    A(2,1).setName("A2");
    p = new FFLTask(A(0,0),A(0,1), nullptr);
    fmm_engine->add_task ( new FFLTask(A(0,0),A(1,1),p) );
    fmm_engine->add_task ( new FFLTask(A(1,1),A(2,1),p) );
  }
  void check_results(){
    if ( !p )
      return;
    cout << "Child count =" << p->child_count << endl;   
  }
}
/*---------------------------------------------------------------------------------------*/
namespace IterationTestSuite{
  void run(){
    
    IterTask::d = new GData (1,1,1); 
    IterTask::d->setHostType(IData::ALL_HOST);
    
    for(int i=0;i<5;i++){
      fmm_engine->add_task( new IterTask(0) );
    }
    
  }
};
/*---------------------------------------------------------------------------------------*/
int main (int argc, char * argv[]){
  DuctTeip_Start(argc,argv);
  FMM_3D::fmm_engine = new FMMContext();

  enum MASKS{
    DataDefinition = 1,
    TaskDefinition = 2,
    TaskCompletion = 4,
    DataCommunication = 8,
    Iteration = 16
  };
  int TestMask = Iteration ;
  
  
  
  if ( TestMask & DataDefinition )
    test_suite_data(); 
  if ( TestMask & TaskDefinition )
    test_task_definition();
  
  if ( TestMask & TaskCompletion )
    TaskTestSuite::test_task_completion();
  
  
  if ( TestMask & DataCommunication)
    DataCommTestSuite::run();    
  if ( TestMask & Iteration )
    IterationTestSuite::run();

  DuctTeip_Finish();
  
  DataCommTestSuite::check_results();
  TaskTestSuite::check_results();
  
  return 0;
}
