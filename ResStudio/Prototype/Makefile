
prog_list=  1 2 3 4 5 6 7 8 9 # 10 11 12 
outdir= ./temp
CPP = mpic++
C = mpicc
SUPERGLUE_DIR=../../../superglue/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format



GCC_COMP_FLAGS=-O3 #-mfma4 -mavx -march=bdver1 
GCC_LINK_FLAGS=-lm  -lstdc++ #-march=bdver1 
INTEL_LINK_FLAGS=
INTEL_COMP_FLAGS=
ACML_DIR=/home/afshin/acml/gnu/acmllib/gfortran64
ACML_LIB=$(ACML_DIR)/lib/libacml.a

EXPORT_PATH := export LD_LIBRARY_PATH='$(ACML_DIR)/lib:$(LD_LIBRARY_PATH)'
BLAS_COMP_FLAGS=-I$(ACML_DIR)/include 
#BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib -nofor-main
BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib -lgfortran 
LINK=mpic++

#########################################################

all : task_gen


task_gen: $(outdir)/test_task_gen.o $(outdir)/test_task_gen  $(shell ls *.hpp) 


#	$(LINK)  $(CPP_LIBS) $(GCC_LINK_FLAGS) $(BLAS_LINK_FLAGS) $(outdir)/test_task_gen.o $(ACML_LIB) $(INTEL_LINK_FLAGS)  -o $(outdir)/test_task_gen 

$(outdir)/test_task_gen: $(outdir)/test_task_gen.o
	$(LINK)  $(CPP_LIBS) $(GCC_LINK_FLAGS)  $(BLAS_LINK_FLAGS) $(outdir)/test_task_gen.o  -o $(outdir)/test_task_gen 
$(outdir)/test_task_gen.o: test_task_gen.cpp
	$(CPP) -c $(GCC_COMP_FLAGS) $(BLAS_COMP_FLAGS) $(SUPERGLUE_FLAGS) test_task_gen.cpp -o $(outdir)/test_task_gen.o 
clean: 
	rm $(outdir)/test*
rebuild: clean all