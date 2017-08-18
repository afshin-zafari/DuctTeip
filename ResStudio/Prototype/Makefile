
outdir          = ./temp
libfile         = ./lib/libductteip.a
libfile_rel     = ./lib/release/libductteip.a
outdir_rel      = ./temp/release

SUPERGLUE_DIR   =/pica/h1/afshin/sg/superglue/include
SUPERGLUE_FLAGS =-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
SOURCE_DIR=./src
HEADER_DIR=./include
#DUCTTEIP_FLAGS=-I$(HEADER_DIR) -DWITH_MPI=1 -I$(SUPERGLUE_DIR) -DUAMD_COMM
DUCTTEIP_FLAGS=-I$(HEADER_DIR) -DWITH_MPI=1 -I$(SUPERGLUE_DIR) 
LINK=mpif90


ifeq ($(CXX),icpc)
#-----------------------Intel Compiler set---------------------
	CPP=mpiicpc
	LINK_FLAGS=-lm -lrt 
	COMP_FLAGS= $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) -std=c++11 -qopt-prefetch -O3  -xCORE-AVX2 
else
#-----------------------GCC Compiler set---------------------
	CPP=mpicxx
	GCOV_FLAGS=-fprofile-arcs -ftest-coverage
	OPTIM_FLAGS=-mavx2 -march=broadwell -mfma -O3 -Wwrite-strings
	SPECIAL_FLAGS=$(OPTIM_FLAGS)     
	LINK_FLAGS=-lm -lrt -lpthread
	COMP_FLAGS= $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) -std=c++11 $(SPECIAL_FLAGS)
endif
#########################################################
headers    :=$(notdir $(shell ls -Sr $(SOURCE_DIR)/*.cpp))
objnames   :=$(headers:%.cpp=%.o)
objects    :=$(addprefix $(outdir)/,$(objnames))	
objects_rel:=$(addprefix $(outdir_rel)/,$(objnames))	

all: $(libfile) $(libfile_rel)

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
	$(info add objects to debug library. )
	$(info ---------------------------------------------------------)
	$(info  )
	@ar rcsv $(libfile) $?

$(objects):  $(outdir)/%.o:  $(SOURCE_DIR)/%.cpp 
	$(info compile $(notdir $<) )
	@$(CPP) -c -o $@ $< $(COMP_FLAGS) -DBUILD=DEBUG -g

#------------------------------------------------------------------------

$(libfile_rel) : $(libfile_rel)($(outdir_rel)/*.o)
	$(info  )
	$(info ---------------------------------------------------------)
	$(info renew library '$(libfile_rel)'.)
	$(info ---------------------------------------------------------)
	$(info  )
	@ranlib $(libfile_rel)

$(libfile_rel)($(outdir_rel)/*.o) : $(objects_rel)
	$(info  )
	$(info ---------------------------------------------------------)
	$(info add objects to release library. )
	$(info ---------------------------------------------------------)
	$(info  )
	@ar rcsv $(libfile_rel) $?

$(objects_rel):  $(outdir_rel)/%.o:  $(SOURCE_DIR)/%.cpp 
	$(info compile $(notdir $<) )
	@$(CPP) -c -o $@ $< $(COMP_FLAGS) -DBUILD=RELEASE
#	$(info  )
#	$(info ---------------------------------------------------------)
#	$(info $? is changed, then compile it.)
#	$(info ---------------------------------------------------------)
#	$(info  )



clean: 
	rm $(outdir)/*.o $(libfile) $(outdir_rel)/*.o $(libfile_rel)

