
ductteip_lib=../lib/libductteip.a
SUPERGLUE_DIR=../../../../superglue/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
HEADER_DIR=../include
DUCTTEIP_FLAGS=-I$(HEADER_DIR)
CPP=mpic++
LINKER=mpif90


GCC_COMP_FLAGS=-march=bdver1 -O3 -mfma4 -mavx
GCC_LINK_FLAGS=-lm  -march=bdver1
ACML_DIR=/home/afshin/acml/acmllib/ifort64_fma4
ACML_LIB=$(ACML_DIR)/lib/libacml.a

EXPORT_PATH := export LD_LIBRARY_PATH='$(ACML_DIR)/lib:$(LD_LIBRARY_PATH)'
BLAS_COMP_FLAGS=-I$(ACML_DIR)/include 
BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib -nofor-main -lacml
GCC_BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib -lacml

#-----------------------Intel Compiler set---------------------
CPP_LIBS=-L/opt/openmpi/1.4.5intel13.1/lib -lstdc++ 
LINK_FLAGS=-lm -lrt -no-ipo $(BLAS_LINK_FLAGS) -lstdc++ #-lpthread
GCC_LINK_FLAGS=-lm -lrt $(BLAS_LINK_FLAGS) -lstdc++ #-lpthread

COMP_FLAGS=-fast -no-ipo -mavx  -opt-prefetch -Os -simd $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS)
GCC_COMP_FLAGS=$(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS)
#########################################################
headers:=$(shell ls -Sr *.hpp)

all: cholesky

cholesky: cholesky.o
	$(info )
	$(info Link with DTLib.)
	$(LINKER) cholesky.o -L../lib -lductteip $(LINK_FLAGS) -o cholesky 
	$(info )

cholesky.o : cholesky.cpp $(headers)
	$(info )
	$(info Compile the source file.)
	$(CPP) -c cholesky.cpp -o cholesky.o $(COMP_FLAGS)
	$(info )

clean: 
	rm *.o cholesky
