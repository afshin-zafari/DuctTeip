#include "gtest/gtest.h"
#include "ductteip.hpp"

int g_argc;
char **g_argv;
TEST(dtMTAdmin,MainAdminMailboxTaskExecutor){
}

int main (int argc , char **argv){
  ::testing::InitGoogleTest(&argc,argv);
  g_argc = argc;
  g_argv = argv;
  return RUN_ALL_TESTS();
}
