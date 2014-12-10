DUCTTEIP_DIR=/pica/h1/afshin/TBP/DuctTeip/ResStudio/Prototype
GTEST_DIR=/pica/h1/afshin/gtest/gtest-1.7.0
GTEST_DIR_INC=$(GTEST_DIR)/include
GTEST_DIR_LIB=$(GTEST_DIR)/lib/.libs
GTEST_MAIN_SRC=$(GTEST_DIR)/gtest_main.cc
GTEST_MAIN_OBJ=$(GTEST_DIR)/gtest_main.o
CFLAGS= -c -I$(DUCTTEIP_DIR) -I$(GTEST_DIR_INC)
CFLAGS_LIB= -L$(GTEST_DIR_LIB) -lgtest -L /opt/openmpi/1.6intel13.1/lib
CXX=mpic++


all: config_utest.o config_utest 

config_utest.o:
	$(CXX) $(CFLAGS) config_utest.cpp -o config_utest.o
$(GTEST_MAIN_OBJ): $(GTEST_MAIN_SRC)
	$(CXX) $(CFLAGS) $(GTEST_MAIN_SRC) -o $@


config_utest:
	$(CXX) $(CFLAGS_LIB) config_utest.o -o $@

clean:
	rm config_utest.o config_utest
.PHONY: all clean
