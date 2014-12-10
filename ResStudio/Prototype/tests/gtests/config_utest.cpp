#include "gtest/gtest.h"
#include "config.hpp"
TEST(ConfigTest, Zero) {
  EXPECT_EQ(1, 1);
}
GTEST_API_ int main(int argc, char **argv) {
  printf("Running main() from config_utest.cc\n");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
