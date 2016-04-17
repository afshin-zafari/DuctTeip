#include "gtest/gtest.h"
#include "ductteip.hpp"

int g_argc;
char **g_argv;
class SampleTest : public Algorithm{ 
private:
  DuctTeip_Data *MM;
  enum KernelKeys{sum};
public:
  void runKernels(IDuctteipTask *t){
    LOG_INFO(LOG_TESTS,"Sum Task run.\n");
    t->setFinished(true);
  }
  string getTaskName(unsigned long key){return string("sum");}
  void taskFinished(IDuctteipTask *task,TimeUnit dur){}
  SampleTest(DuctTeip_Data *X){
    setParent(this);    
    X->setDataHandle(createDataHandle());    
    MM=X->clone();
    MM->setParent(this);
    MM->configure();    
    addInOutData(MM);
    DuctTeip_Data M=*MM;
    IData &C=*MM;
    string sum_st("sum");
    
    AddTask ((IContext*)this,sum_st,0L,C(0,0),C(0,1));
    AddTask ((IContext*)this,sum_st,0L,C(0,1),C(1,0)); 
    LOG_INFO(LOG_TESTS,"Sum Tasks submitted.\n");
    
  }
};
class dtMTAdmin: public testing::Test{
protected:
  DuctTeip_Data *A;
  SampleTest *t;
  void SetUp(){
    A = new DuctTeip_Data(48,48);
    t=new SampleTest (static_cast<DuctTeip_Data *>(A));    
  }
  void TearDown(){
    //delete A;
    //delete t;
  }
};

TEST(dtMTAdmin,MainAdmin){
  DuctTeip_Data *A;
  SampleTest *t;
  dtEngine.setThreadModel(0);
  int tm=  dtEngine.getThreadModel();
  EXPECT_EQ(0,tm);
  DuctTeip_Start(g_argc,g_argv);
  A = new DuctTeip_Data(48,48);
  t=new SampleTest (static_cast<DuctTeip_Data *>(A));    

  
  long main_thrd =dtEngine.getThreadId(engine::MAIN_THREAD );
  long admin_thrd=dtEngine.getThreadId(engine::ADMIN_THREAD);

  EXPECT_NE(main_thrd,admin_thrd);

  DuctTeip_Finish();
  LOG_INFO(LOG_TESTS,"SampleClass test finished.\n");
  //delete A;
  //delete t;

}

int main (int argc , char **argv){
  ::testing::InitGoogleTest(&argc,argv);
  g_argc = argc;
  g_argv = argv;
  return RUN_ALL_TESTS();
}
