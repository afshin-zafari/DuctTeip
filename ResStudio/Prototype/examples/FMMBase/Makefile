
DUCTTEIP_DIR   = /pica/h1/afshin/Damavand/D4/DuctTeip/ResStudio/Prototype
ductteip_lib   = $(DUCTTEIP_DIR)/lib/libductteip.a
HEADER_DIR     = $(DUCTTEIP_DIR)/include
DUCTTEIP_FLAGS = -I$(HEADER_DIR)

SUPERGLUE_DIR   = /pica/h1/afshin/sg/superglue/include
SUPERGLUE_FLAGS = -pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
FMMBASE_FLAGS   = -Iinclude

ifeq ($(CXX),icpc)
#-----------------------Intel Compiler set---------------------
	CPP=mpiicpc
	LINKER=mpiicpc

	LINK_FLAGS=-lm -lrt -no-ipo -lstdc++ -Wl,--allow-multiple-definition 
	INTEL_OPTIM=-no-ipo -qopt-prefetch -Os -simd -mavx
	COMP_FLAGS= -std=c++11 -g $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS) $(FMMBASE_FLAGS)
else
	ACML_DIR=/pica/h1/afshin/acml/gnu/acmllib/gfortran64
	ACML_LIB=$(ACML_DIR)/lib/libacml.a
	BLAS_COMP_FLAGS=-I$(ACML_DIR)/include 

	GCOV_FLAGS=-fprofile-arcs -ftest-coverage
	OPTIM_FLAGS=-mavx  -Wwrite-strings -march=bdver1 -O3 -mfma4
	SPECIAL_FLAGS=$(OPTIM_FLAGS)     


	LINKER=mpic++
	CPP=mpic++

	BLAS_LINK_FLAGS=-L$(ACML_DIR)/lib -lacml
	LINK_FLAGS=-lm -lrt -march=bdver1 $(BLAS_LINK_FLAGS) -lstdc++ -lpthread -Wl,-rpath=$(ACML_DIR)/lib  -Wl,--allow-multiple-definition #-fprofile-arcs 
	COMP_FLAGS= $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) $(BLAS_COMP_FLAGS) $(SPECIAL_FLAGS)
endif
#########################################################
SOURCE_DIR  = ./src
outdir      = ./bin
app         = $(outdir)/fmmbase
source     := $(notdir $(shell ls -Sr $(SOURCE_DIR)/*.cpp))
objnames   := $(source:%.cpp=%.o)
objects    := $(addprefix $(outdir)/,$(objnames))	

all: $(app)

$(app): $(objects) 
	$(info  )
	$(info ---------------------------------------------------------)
	$(info link to $(app). )
	$(info ---------------------------------------------------------)
	$(info  )
	@$(LINKER) $(objects) -L../lib $(ductteip_lib) $(LINK_FLAGS) -o $(app)


$(objects):  $(outdir)/%.o:  $(SOURCE_DIR)/%.cpp 
	$(info compile $(notdir $<) )
	@$(CPP) $(COMP_FLAGS) -c $< -o $@

clean: 
	rm -f bin/*.o $(app) bin/*file*
