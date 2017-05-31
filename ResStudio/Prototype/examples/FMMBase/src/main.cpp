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
void test_task_submission(){}//todo
/*---------------------------------------------------------------------------------------*/
void test_task_execution(){}//todo
/*---------------------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------------------*/
int main (int argc, char * argv[]){
  DuctTeip_Start(argc,argv);
  
  FMM_3D::fmm_engine = new FMMContext();
  test_suite_data(); 
  test_task_definition();
  test_task_completion();
  DuctTeip_Finish();
  cout << "Chil count of Parent task: " << p->child_count  << endl;
  return 0;
}
