
outdir= ./temp
libfile=./lib/libductteip.a
SUPERGLUE_DIR=../../../superglue/include
SUPERGLUE_FLAGS=-pthread -I$(SUPERGLUE_DIR)  #-pedantic -Wno-long-long -Wno-format
SOURCE_DIR=./src
HEADER_DIR=./include
DUCTTEIP_FLAGS=-I$(HEADER_DIR)
CPP=mpic++
LINK=mpif90

#-----------------------Intel Compiler set---------------------
LINK_FLAGS=-lm -lrt #-lpthread
COMP_FLAGS= $(SUPERGLUE_FLAGS) $(DUCTTEIP_FLAGS) #-mavx  -opt-prefetch -Os -simd
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
