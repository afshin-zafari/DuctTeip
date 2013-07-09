
prog_list=  1 2 3 4 5 6 7 8 # 9 10 11 12 
outdir= ./temp
CPP = /opt/SUNWspro/bin/CC
CPP_LIBS =  -L/opt/SUNWspro/lib/ -lCstd -lCrun
C = /opt/SUNWspro/bin/cc

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
    CPP = /usr/bin/mpic++
    CPP_LIBS =  -L/usr/lib/  -lmpi -lstdc++
    C = /usr/bin/mpic++
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

define rule_tests
$(outdir)/test$(1): $(outdir)/main$(1).o $(shell ls *.hpp) 
	$(CPP) $(CPP_LIBS)  $(outdir)/main$(1).o -o $(outdir)/test$(1) 
$(outdir)/main$(1).o: main$(1).cpp $(shell ls *.hpp) 
	$(CPP) -c main$(1).cpp -o $(outdir)/main$(1).o 
endef

#########################################################

all: $(foreach i, $(prog_list), $(outdir)/test$(i) ) 

$(foreach i, $(prog_list), $(eval $(call rule_tests,$(i))))


clean: 
	rm $(outdir)/*.o $(outdir)/test*
