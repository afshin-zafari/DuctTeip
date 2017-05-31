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
void test_task_definition(){}//todo
/*---------------------------------------------------------------------------------------*/
void test_task_submission(){}//todo
/*---------------------------------------------------------------------------------------*/
void test_task_execution(){}//todo
/*---------------------------------------------------------------------------------------*/
void test_task_completion(){}//todo
/*---------------------------------------------------------------------------------------*/
int main (int argc, char * argv[]){
  DuctTeip_Start(argc,argv);
  
  FMM_3D::fmm_engine = new FMMContext();
  test_suite_data(); 
  
  DuctTeip_Finish();
  return 0;
}
