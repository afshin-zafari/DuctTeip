
ductteip_lib=/pica/h1/afshin/Damavand/D4/DuctTeip/ResStudio/Prototype/lib/libductteip.a
SUPERGLUE_DIR=/pica/h1/afshin/sg/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
HEADER_DIR=../../include
DUCTTEIP_FLAGS=-I$(HEADER_DIR) -std=c++11

COMP=gcc
CPP=mpic++
OUT=debug
BLAS=openblas

ifeq ($(BLAS),openblas)
	BLAS_DIR=/sw/libs/openblas/0.2.19/rackham
	BLAS_LIB=$(BLAS_DIR)/lib
	BLAS_LINK_FLAGS=-L$(BLAS_DIR)/lib -lopenblas
	BLAS_COMP_FLAGS=-I$(BLAS_DIR)/include -DUSE_OPENBLAS -UUSE_MKL
endif

ifeq ($(BLAS),MKL)
	MKL_ROOT=/sw/comp/intel/compilers_and_libraries_2017.4.196/linux/mkl
	MKL_INC=-isystem $(MKL_ROOT)/include
	MKL_LIBS= \
	-Wl,--start-group \
	$(MKL_ROOT)/lib/intel64/libmkl_intel_lp64.a \
	$(MKL_ROOT)/lib/intel64/libmkl_sequential.a \
	$(MKL_ROOT)/lib/intel64/libmkl_core.a \
	-ldl \
	-Wl,--end-group

	BLAS_LINK_FLAGS=$(MKL_LIBS)
	BLAS_COMP_FLAGS=$(MKL_INC) -DUSE_MKL -UUSE_OPENBLAS
endif


ifeq ($(OUT),debug)
     ductteip_lib=/pica/h1/afshin/Damavand/D4/DuctTeip/ResStudio/Prototype/lib/libductteip.a
     OUT_FLAG= -g
else
     ductteip_lib=/pica/h1/afshin/Damavand/D4/DuctTeip/ResStudio/Prototype/lib/release/libductteip.a
     OUT_FLAG=-DDEBUG=0 -DBUILD=RELEASE
endif

DUCTTEIP_FLAGS += $(OUT_FLAG)

ifeq ($(CXX),icpc)
#-----------------------Intel Compiler set---------------------
	CPP=mpiicpc
	LINKER=mpiicpc

	LINK_FLAGS=-mt_mpi -lrt -no-ipo -lstdc++ $(BLAS_LINK_FLAGS) -lm -Wl,-rpath=$(BLAS_DIR)/lib -Wl,--allow-multiple-definition 
#	LINK_FLAGS=-lrt -no-ipo -lstdc++ $(MKL_LINK_FLAGS) -lm -Wl,--allow-multiple-definition 
	COMP_FLAGS= -no-ipo -qopt-prefetch -Os -simd -xCORE-AVX2  $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS)
	COMP_FLAGS= -no-ipo -qopt-prefetch -Os -simd -xCORE-AVX2  $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS)
#	COMP_FLAGS=-fast -no-ipo -qopt-prefetch -Os -simd -xCORE-AVX2  $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(MKL_COMP_FLAGS)
else
	GCOV_FLAGS=-fprofile-arcs -ftest-coverage
	OPTIM_FLAGS=-mavx  -Wwrite-strings -march=broadwell -O3 -mfma
	OPTIM_FLAGS=
	SPECIAL_FLAGS=$(OPTIM_FLAGS)     


	LINKER=mpic++

	BLAS_LINK_FLAGS=-L$(BLAS_DIR)/lib -lopenblas
#	LINK_FLAGS=-lm -lrt -march=bdver1 $(BLAS_LINK_FLAGS) -lstdc++ -lpthread -Wl,-rpath=$(ACML_DIR)/lib  -Wl,--allow-multiple-definition #-fprofile-arcs 
	LINK_FLAGS=-lm -lrt $(BLAS_LINK_FLAGS) -lstdc++ -lpthread -Wl,-rpath=$(BLAS_DIR)/lib  -Wl,--allow-multiple-definition #-fprofile-arcs 
	COMP_FLAGS= $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS) $(OUT_FLAG) $(SPECIAL_FLAGS) 
endif
#########################################################
headers:=$(shell ls -Sr *.hpp)

all: bin/cholesky_$(COMP)

bin/cholesky_$(COMP): bin/cholesky.o bin/main.o
	$(info )
	$(info Link with DTLib.)
	$(LINKER) bin/cholesky.o bin/main.o $(ductteip_lib) $(LINK_FLAGS) -o bin/cholesky_$(COMP) 
	$(info )

bin/cholesky.o : cholesky.cpp $(headers)
	$(info )
	$(info Compile the source file ($(OUT)).)
	$(CPP) -c cholesky.cpp -o bin/cholesky.o $(COMP_FLAGS)
	$(info )

bin/main.o : main.cpp $(headers)
	$(info )
	$(info Compile the source file.)
	$(CPP) -c main.cpp -o bin/main.o $(COMP_FLAGS)
	$(info )



clean: 
	rm -f bin/*.o bin/cholesky_$(COMP) bin/*file*
