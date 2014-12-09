
prog_list=  1 2 3 4 5 6 7 8 9 # 10 11 12 
outdir= ./temp
CPP = /opt/SUNWspro/bin/CC
CPP_LIBS =  -L/opt/SUNWspro/lib/ -lCstd -lCrun
C = /opt/SUNWspro/bin/cc
SUPERGLUE_DIR=../../../superglue/include
SUPERGLUE_FLAGS=-I $(SUPERGLUE_DIR) -pthread #-pedantic -Wno-long-long -Wno-format


ifdef LINUX
    CPP = /usr/bin/cc
    CPP_LIBS =  -L/usr/lib/  -lstdc++
    C = /usr/bin/cc
endif
ifdef UBUNTU
    CPP = /usr/bin/g++
    CPP_LIBS =  -L/usr/lib/  -lstdc++
    C = /usr/bin/g++
endif
ifdef UBUNTUMPI
    CPP = mpic++
    CPP_LIBS =  -lmpi -lstdc++
    C = mpic++
endif
ifdef SCILINUX
    CPP = /usr/bin/gcc
    CPP_LIBS =  -L/usr/lib/  -lstdc++
    C = /usr/bin/gcc
endif
ifdef KALKYL
    CPP = /bubo/sw/comp/gcc/4.7.2_kalkyl/bin/gcc
    CPP_LIBS =  -L/bubo/sw/comp/gcc/4.7.2_kalkyl/lib -lstdc++
    C = /bubo/sw/comp/gcc/4.7.2_kalkyl/bin/gcc
endif
ifdef KALKYLMPI	
    CPP = /opt/openmpi/1.4.5gcc4.4/bin/mpic++
    CPP_LIBS =  -L/opt/openmpi/1.4.5gcc4.4/lib/  -lmpi -lstdc++
    C = /opt/openmpi/1.4.5gcc4.4/bin/mpic++
endif

define rule_tests
$(outdir)/test$(1): $(outdir)/main$(1).o $(shell ls *.hpp) 
	$(CPP) $(CPP_LIBS)  $(outdir)/main$(1).o -o $(outdir)/test$(1) 
$(outdir)/main$(1).o: main$(1).cpp $(shell ls *.hpp) 
	$(CPP) -c main$(1).cpp -o $(outdir)/main$(1).o 
endef

#########################################################

all: $(foreach i, $(prog_list), $(outdir)/test$(i) ) 

$(foreach i, $(prog_list), $(eval $(call rule_tests,$(i))))

task_gen: $(outdir)/test_task_gen.o $(outdir)/test_task_gen  $(shell ls *.hpp) 

$(outdir)/test_task_gen: $(outdir)/test_task_gen.o
	$(CPP)  $(CPP_LIBS) -lm  $(outdir)/test_task_gen.o -o $(outdir)/test_task_gen 
$(outdir)/test_task_gen.o: test_task_gen.cpp
	$(CPP) -c $(SUPERGLUE_FLAGS) test_task_gen.cpp -o $(outdir)/test_task_gen.o 
clean: 
	rm $(outdir)/*.o $(outdir)/test*
