
prog_list=  1 2 3 4 5 6 7 8 9 10 11 12 
outdir= ./temp
define rule_tests
$(outdir)/test$(1): $(outdir)/main$(1).o $(shell ls *.hpp) 
	g++ -lstdc++ -L/usr/lib/  $(outdir)/main$(1).o -o $(outdir)/test$(1) 
$(outdir)/main$(1).o: main$(1).cpp $(shell ls *.hpp) 
	g++ -c main$(1).cpp -o $(outdir)/main$(1).o 
endef

#########################################################

all: $(foreach i, $(prog_list), $(outdir)/test$(i) ) 

$(foreach i, $(prog_list), $(eval $(call rule_tests,$(i))))



clean: 
	rm $(outdir)/*.o $(outdir)/test*
