
ductteip_lib=../lib/libductteip.a
SUPERGLUE_DIR=../../../../superglue/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
HEADER_DIR=../include
DUCTTEIP_FLAGS=-I$(HEADER_DIR)
CPP=mpic++

ifeq ($(CXX),icpc)
#-----------------------Intel Compiler set---------------------
	ACML_DIR=/home/afshin/acml/acmllib/ifort64_fma4
	ACML_LIB=$(ACML_DIR)/lib/libacml.a
	EXPORT_PATH := export LD_LIBRARY_PATH='$(ACML_DIR)/lib:$(LD_LIBRARY_PATH)'
	BLAS_COMP_FLAGS=-I$(ACML_DIR)/include 

	LINKER=mpif90

	BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib  -lacml -nofor-main
	LINK_FLAGS=-lm -lrt -no-ipo -lstdc++ $(BLAS_LINK_FLAGS) 
	COMP_FLAGS=-fast -no-ipo -opt-prefetch -Os -simd $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS) #-mavx 
else
	ACML_DIR=/pica/h1/afshin/acml/gnu/acmllib/gfortran64
	ACML_LIB=$(ACML_DIR)/lib/libacml.a
	BLAS_COMP_FLAGS=-I$(ACML_DIR)/include 

	GCOV_FLAGS=-fprofile-arcs -ftest-coverage
	OPTIM_FLAGS=-mavx  -Wwrite-strings -march=bdver1 -O3 -mfma4
	SPECIAL_FLAGS=$(OPTIM_FLAGS)     


	LINKER=mpic++

	BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib -lacml
	LINK_FLAGS=-lm -lrt -march=bdver1 $(BLAS_LINK_FLAGS) -lstdc++ -lpthread -Wl,-rpath=$(ACML_DIR)/lib  #-fprofile-arcs 
	COMP_FLAGS= $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS) $(SPECIAL_FLAGS)
endif
#########################################################
headers:=$(shell ls -Sr *.hpp)

all: bin/cholesky

bin/cholesky: bin/cholesky.o bin/dt_util.o
	$(info )
	$(info Link with DTLib.)
	$(LINKER) $? -L../lib -lductteip $(LINK_FLAGS) -o bin/cholesky 
	$(info )

bin/cholesky.o : cholesky.cpp $(headers)
	$(info )
	$(info Compile the source file.)
	$(CPP) -c cholesky.cpp -o bin/cholesky.o $(COMP_FLAGS)
	$(info )

bin/dt_util.o : dt_util.cpp $(headers)
	$(info )
	$(info Compile the source file.)
	$(CPP) -c dt_util.cpp -o bin/dt_util.o $(COMP_FLAGS)
	$(info )



clean: 
	rm bin/*.o bin/cholesky bin/*file*
