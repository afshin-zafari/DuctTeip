#include "gtest/gtest.h"
#include "config.hpp"
TEST(ConfigTest,UsingDefaults){
  char *argv[5]={"app","-P","2","-N","1024"};
  int argc =5;
  Config cfg(&argc,argv);
  EXPECT_EQ(1024,cfg.getYDimension());  
}
TEST(ConfigDeathTest, MissedMandatoryParams) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  char *argv[2]={"app","-P"};
  int argc =2;
 
  EXPECT_DEATH({ Config cfg(&argc,argv);},".*param.*");
}

/*---------------------------------------------------*/
GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
