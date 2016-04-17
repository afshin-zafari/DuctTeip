
prog_list=  1 2 3 4 5 6 7 8 9 # 10 11 12 
outdir= ./temp
CPP = /opt/SUNWspro/bin/CC
CPP_LIBS =  -L/opt/SUNWspro/lib/ -lCstd -lCrun
C = /opt/SUNWspro/bin/cc
SUPERGLUE_DIR=../../../superglue/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format



ifdef TINTINMPI	
    CPP = /opt/openmpi/1.6.4gcc4.7/bin/mpic++
    CPP_LIBS =  -L/opt/openmpi/1.6.4cc4.7/lib/  -lmpi -lstdc++
    C = /opt/openmpi/1.6.4gcc4.7/bin/mpic++
endif
GCC_COMP_FLAGS=-march=bdver1 -O3 -mfma4 -mavx
GCC_LINK_FLAGS=-lm  -march=bdver1
INTEL_LINK_FLAGS=
INTEL_COMP_FLAGS=
OPEN64_COMP_FLAGS=
OPEN64_LINK_FLAGS=
ACML_DIR=/home/afshin/acml/acmllib/ifort64_fma4
ACML_LIB=$(ACML_DIR)/lib/libacml.a

EXPORT_PATH := export LD_LIBRARY_PATH='$(ACML_DIR)/lib:$(LD_LIBRARY_PATH)'
BLAS_COMP_FLAGS=-I$(ACML_DIR)/include 
BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib -nofor-main
LINK=$(CPP)
#-----------------------Intel Compiler set---------------------
ifdef TINTININTELMPI	
    GCC_COMP_FLAGS=
    GCC_LINK_FLAGS=
    INTEL_LINK_FLAGS=-lm -lrt
    INTEL_COMP_FLAGS=-fast -mavx  -opt-prefetch -Os -simd
    CPP = mpic++
    LINK=mpif90
    CPP_LIBS =  -L /opt/openmpi/1.6intel13.1/lib -lstdc++ # 
    C = mpic++
endif
ifdef TAU	
    GCC_COMP_FLAGS=
    GCC_LINK_FLAGS=
    INTEL_LINK_FLAGS=-lm -lrt
    INTEL_COMP_FLAGS=-fast -mavx  -opt-prefetch -Os -simd
    CPP = tau_cxx.sh -tau_options=
    LINK=tau_f90.sh -tau_options=
    CPP_LIBS =  -L /opt/openmpi/1.6intel13.1/lib -lstdc++ # 
    C = tau_cxx.sh -tau_options=
endif



#-----------------------Open64 Compiler set---------------------
ifdef TINTINOPEN64
    GCC_COMP_FLAGS=
    GCC_LINK_FLAGS=
    INTEL_LINK_FLAGS=
    INTEL_COMP_FLAGS=
    OPEN64_COMP_FLAGS=-mavx -mfma4 -mcpu=bdver1 -mtune=bdver1 -Ofast -mso 
    OPEN64_LINK_FLAGS=-ipa 
    CPP = mpic++
    CPP_LIBS =  -L /opt/openmpi/1.6.4open64amd-4.5/lib
    C = mpic++
endif



define rule_tests
$(outdir)/test$(1): $(outdir)/main$(1).o $(shell ls *.hpp) 
	$(CPP) $(CPP_LIBS)  $(outdir)/main$(1).o -o $(outdir)/test$(1) 
$(outdir)/main$(1).o: main$(1).cpp $(shell ls *.hpp) 
	$(CPP) -c main$(1).cpp -o $(outdir)/main$(1).o 
endef
ifdef TV
	DEBUG_COMP=-pg 
	DEBUG_LINK=-pg
endif

#########################################################

all: $(foreach i, $(prog_list), $(outdir)/test$(i) ) 

$(foreach i, $(prog_list), $(eval $(call rule_tests,$(i))))

task_gen: $(outdir)/test_task_gen.o $(outdir)/test_task_gen  $(shell ls *.hpp) 

$(outdir)/test_task_gen: $(outdir)/test_task_gen.o
	$(LINK)  $(CPP_LIBS) $(GCC_LINK_FLAGS) $(OPEN64_LINK_FLAGS) $(BLAS_LINK_FLAGS) $(outdir)/test_task_gen.o $(ACML_LIB) $(INTEL_LINK_FLAGS) $(DEBUG_LINK) -o $(outdir)/test_task_gen 
$(outdir)/test_task_gen.o: test_task_gen.cpp
	$(CPP) -c $(DEBUG_COMP) $(GCC_COMP_FLAGS) $(INTEL_COMP_FLAGS) $(OPEN64_COMP_FLAGS) $(SUPERGLUE_FLAGS) $(BLAS_COMP_FLAGS)  test_task_gen.cpp -o $(outdir)/test_task_gen.o 
clean: 
	rm $(outdir)/*.o $(outdir)/test*
