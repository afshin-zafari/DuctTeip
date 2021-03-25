
ductteip_lib=../Prototype/lib/libductteip.a
SUPERGLUE_DIR=../../../superglue/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
HEADER_DIR=../Prototype/include
DUCTTEIP_FLAGS=-I$(HEADER_DIR) -std=c++11 -DWITH_MPI=10

COMP=gcc
CPP=mpic++
OUT=debug


ifeq ($(OUT),debug)
     ductteip_lib=../Prototype/lib/libductteip.a
     OUT_FLAG= -g
else
     ductteip_lib=../Prototype/lib/release/libductteip.a
     OUT_FLAG=-DDEBUG=0 -DBUILD=RELEASE
endif

DUCTTEIP_FLAGS += $(OUT_FLAG)

GCOV_FLAGS=-fprofile-arcs -ftest-coverage
OPTIM_FLAGS=-mavx  -Wwrite-strings -march=broadwell -O3 -mfma
OPTIM_FLAGS=
SPECIAL_FLAGS=$(OPTIM_FLAGS)     


LINKER=mpicc

#	LINK_FLAGS=-lm -lrt -march=bdver1 $(BLAS_LINK_FLAGS) -lstdc++ -lpthread -Wl,-rpath=$(ACML_DIR)/lib  -Wl,--allow-multiple-definition #-fprofile-arcs 
LINK_FLAGS= -lrt  -lstdc++ -Wl,--allow-multiple-definition
COMP_FLAGS= $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(OUT_FLAG) $(SPECIAL_FLAGS) 
#########################################################
headers:=$(shell ls -Sr *.hpp)

all: bin/test_$(COMP)

bin/test_$(COMP): bin/main.o
	$(info )
	$(info Link with DTLib.)
	$(LINKER)  bin/main.o $(ductteip_lib) $(LINK_FLAGS) -o bin/test_$(COMP) 
	$(info )

bin/main.o : main.cpp $(headers)
	$(info )
	$(info Compile the source file.)
	$(CPP) -c main.cpp -o bin/main.o $(COMP_FLAGS)
	$(info )



clean: 
	rm -f bin/*.o bin/test_$(COMP) bin/*file*
