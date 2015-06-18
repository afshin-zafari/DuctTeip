
prog_list=  1 2 3 4 5 6 7 8 9 # 10 11 12 
outdir= ./temp
libfile=./lib/libductteip.a
SUPERGLUE_DIR=../../../superglue/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
SOURCE_DIR=./src
HEADER_DIR=./include
DUCTTEIP_FLAGS=-I$(HEADER_DIR)
CPP=mpic++
LINK=mpif90


GCC_COMP_FLAGS=-march=bdver1 -O3 -mfma4 -mavx
GCC_LINK_FLAGS=-lm  -march=bdver1
ACML_DIR=/home/afshin/acml/acmllib/ifort64_fma4
ACML_LIB=$(ACML_DIR)/lib/libacml.a

EXPORT_PATH := export LD_LIBRARY_PATH='$(ACML_DIR)/lib:$(LD_LIBRARY_PATH)'
BLAS_COMP_FLAGS=-I$(ACML_DIR)/include 
BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib -nofor-main

#-----------------------Intel Compiler set---------------------
LINK_FLAGS=-lm -lrt
COMP_FLAGS=-fast -mavx  -opt-prefetch -Os -simd -I$(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS)
#########################################################
headers:=$(notdir $(shell ls -Sr $(HEADER_DIR)/*.hpp))
objnames:=$(headers:%.hpp=%.o)
objects:=$(addprefix $(outdir)/,$(objnames))	
all: $(libfile)

$(libfile) : $(libfile)($(outdir)/*.o)
	$(info  )
	$(info ---------------------------------------------------------)
	$(info renew library '$(libfile)'.)
	$(info ---------------------------------------------------------)
	$(info  )
	@ranlib $(libfile)

$(libfile)($(outdir)/*.o) : $(objects)
	$(info  )
	$(info ---------------------------------------------------------)
	$(info add objects to library. )
	$(info ---------------------------------------------------------)
	$(info  )
	@ar rcsv $(libfile) $?

$(objects):  $(outdir)/%.o:  $(SOURCE_DIR)/%.cpp $(HEADER_DIR)/%.hpp
	$(info  )
	$(info ---------------------------------------------------------)
	$(info $? is changed, then compile it.)
	$(info ---------------------------------------------------------)
	$(info  )
	$(info mpic++ -c $< -o $@)
	@$(CPP) -c -o $@ $< $(COMP_FLAGS)


clean: 
	rm $(outdir)/*.o $(libfile)
