
ductteip_lib=/pica/h1/afshin/Damavand/D4/DuctTeip/ResStudio/Prototype/lib/libductteip.a
SUPERGLUE_DIR=/pica/h1/afshin/sg/superglue/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
HEADER_DIR=../../include
DUCTTEIP_FLAGS=-I$(HEADER_DIR) -std=c++11
BLAS_DIR=/sw/libs/openblas/0.2.19/rackham
BLAS_LIB=$(BLAS_DIR)/lib
BLAS_LINK_FLAGS=-L$(BLAS_DIR)/lib -lopenblas
BLAS_COMP_FLAGS=-I$(BLAS_DIR)/include 


CPP=mpic++
OUT=release
ifeq ($(OUT),debug)
     ductteip_lib=/pica/h1/afshin/Damavand/D4/DuctTeip/ResStudio/Prototype/lib/libductteip.a
     OUT_FLAG=-g      
else
     ductteip_lib=/pica/h1/afshin/Damavand/D4/DuctTeip/ResStudio/Prototype/lib/release/libductteip.a
     OUT_FLAG=-DDEBUG=0
endif

ifeq ($(CXX),icpc)
#-----------------------Intel Compiler set---------------------
	CPP=mpiicpc
	LINKER=mpiicpc

	LINK_FLAGS=-lm -lrt -no-ipo -lstdc++ $(BLAS_LINK_FLAGS) -Wl,-rpath=$(BLAS_DIR)/lib -Wl,--allow-multiple-definition 
	COMP_FLAGS=-fast -no-ipo -qopt-prefetch -Os -simd -mavx $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS)
else
	GCOV_FLAGS=-fprofile-arcs -ftest-coverage
	OPTIM_FLAGS=-mavx  -Wwrite-strings -march=broadwell -O3 -mfma
	SPECIAL_FLAGS=$(OPTIM_FLAGS)     


	LINKER=mpic++

	BLAS_LINK_FLAGS=-L$(BLAS_DIR)/lib -lopenblas
#	LINK_FLAGS=-lm -lrt -march=bdver1 $(BLAS_LINK_FLAGS) -lstdc++ -lpthread -Wl,-rpath=$(ACML_DIR)/lib  -Wl,--allow-multiple-definition #-fprofile-arcs 
	LINK_FLAGS=-lm -lrt $(BLAS_LINK_FLAGS) -lstdc++ -lpthread -Wl,-rpath=$(BLAS_DIR)/lib  -Wl,--allow-multiple-definition #-fprofile-arcs 
	COMP_FLAGS= $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS) $(OUT_FLAG) #$(SPECIAL_FLAGS) 
endif
#########################################################
headers:=$(shell ls -Sr *.hpp)

all: bin/cholesky

bin/cholesky: bin/cholesky.o bin/main.o
	$(info )
	$(info Link with DTLib.)
	$(LINKER) bin/cholesky.o bin/main.o $(ductteip_lib) $(LINK_FLAGS) -o bin/cholesky 
	$(info )

bin/cholesky.o : cholesky.cpp $(headers)
	$(info )
	$(info Compile the source file.)
	$(CPP) -c cholesky.cpp -o bin/cholesky.o $(COMP_FLAGS)
	$(info )

bin/main.o : main.cpp $(headers)
	$(info )
	$(info Compile the source file.)
	$(CPP) -c main.cpp -o bin/main.o $(COMP_FLAGS)
	$(info )



clean: 
	rm -f bin/*.o bin/cholesky bin/*file*
