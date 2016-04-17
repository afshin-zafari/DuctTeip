DUCTTEIP_DIR=/pica/h1/afshin/TBP/DuctTeip/ResStudio/Prototype
DUCTTEIP_INC=$(DUCTTEIP_DIR)/include
DUCTTEIP_LIB=$(DUCTTEIP_DIR)/lib

GTEST_DIR=/pica/h1/afshin/gtest/gtest-1.7.0
GTEST_DIR_INC=$(GTEST_DIR)/include
GTEST_DIR_LIB=$(GTEST_DIR)/lib/.libs

ACML_DIR=/home/afshin/acml/acmllib/ifort64_fma4
ACML_LIB=$(ACML_DIR)/lib/libacml.a
EXPORT_PATH := export LD_LIBRARY_PATH='$(ACML_DIR)/lib:$(LD_LIBRARY_PATH)'
BLAS_COMP_FLAGS=-I$(ACML_DIR)/include 
LINKER=mpif90
BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib  -lacml -nofor-main


GTEST_MAIN_SRC=$(GTEST_DIR)/gtest_main.cc
GTEST_MAIN_OBJ=$(GTEST_DIR)/gtest_main.o
SUPERGLUE_INC=/pica/h1/afshin/TBP/superglue/include
CFLAGS= -std=c++11 -c -I$(DUCTTEIP_INC) -I$(GTEST_DIR_INC) -I$(SUPERGLUE_INC) $(BLAS_COMP_FLAGS)
CFLAGS_LIB= -L$(GTEST_DIR_LIB) -L$(DUCTTEIP_LIB) -lgtest -lductteip -lm -lrt -lstdc++ $(BLAS_LINK_FLAGS)
CXX=mpic++


outdir=../temp
appdir=../bin
TEST_CASES:=$(notdir $(wildcard *.cpp) )
objnames:=$(TEST_CASES:%.cpp=%.o)
objects:=$(addprefix $(outdir)/,$(objnames))
appnames:=$(TEST_CASES:%.cpp=%.test)
apps:=$(addprefix $(appdir)/,$(appnames))

all: $(apps)


$(objects): $(outdir)/%.o: %.cpp 
	$(CXX) $(CFLAGS) $< -o $@

$(apps): $(appdir)/%.test: $(outdir)/%.o 
	$(info )
	@$(LINKER) $< $(CFLAGS_LIB) -o $@
	$(info --------------------------------------------------)
	$(info --> $(@F))
	$(info --------------------------------------------------)


clean:
	@rm -r $(apps) $(objects) $(outdir)/* > /dev/null 2>&1
.PHONY: all clean 
