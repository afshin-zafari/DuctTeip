
ifeq ($(FC),gnu)
    F90 = /it/sw/gnu/bin/gfortran 
    MPIF90=/opt/SUNWhpc/bin/mpif90
    PP.FLAGS= -DDEBUG_LEVEL=0 -DRBF_VERSION=3
    F90.FLAGS = -frecursive #-m64 
else
#    F90 = /opt/SUNWspro-12u2/bin/f90
    F90 = /opt/SUNWspro/bin/f90
    MPIF90=/opt/SUNWhpc/bin/mpif90
    PP.FLAGS= -F -e -w0 -DDEBUG_LEVEL=0 -DVIZ_LEVEL=0 -DRBF_VERSION=3 -DTIMING_TRACE=1 -DSAVE_DATA=0 
    F90.FLAGS =  -stackvar -O0 -g  #-m64 
endif

#CPP = /opt/SUNWspro-12u2/bin/CC
CPP = /opt/SUNWspro/bin/CC
#CPP_LIBS =  -L/opt/SUNWspro-12u2/lib/ -lCstd -lCrun
CPP_LIBS =  -L/opt/SUNWspro/lib/ -lCstd -lCrun
#C = /opt/SUNWspro-12u2/bin/cc
C = /opt/SUNWspro/bin/cc

ifdef UBUNTU
    F90 = mpif90
    MPIF90 =mpif90 
    PP.FLAGS=-E -DDEBUG_LEVEL=0 -DUPPMAX=1 -DRBF_VERSION=3 -DTIMING_TRACE=1 -DSAVE_DATA=0
    F90.FLAGS = #-reentrancy  #-m64 
    CPP = cc
    C = cc
    CPP_LIBS = -lstdc++ -lpthread #-lCstd -lCrun
endif
ifdef UPPMAX
    F90 = mpif90
    MPIF90 = mpif90 
    PP.FLAGS=-E -DDEBUG_LEVEL=0 -DUPPMAX=1 -DRBF_VERSION=3 -DTIMING_TRACE=1 -DSAVE_DATA=0
    F90.FLAGS = #-reentrancy  #-m64 
    CPP = $(CXX)
    C = $(CC)
    CPP_LIBS = -lstdc++ -lpthread #-lCstd -lCrun
endif

F90.LINK = $(F90) $(F90.FLAGS)
MPIF90.LINK = $(MPIF90) $(F90.FLAGS)
F90.COMPILE = $(F90) $(F90.FLAGS) -c
MPIF90.COMPILE = $(MPIF90) $(F90.FLAGS) -c
CPP.COMPILE = $(CPP) $(CPP.FLAGS) -c
CPP.FLAGS = # -m64
C.COMPILE = $(C) $(C.FLAGS) -c
C.FLAGS =  #-m64


OBJECTS = highrestimer.o timer.o constants.o fp.o fld.o rbf.o const.o eqconst.o out.o pts.o fval.o expr2.o eps.o dist.o geometry_class.o expression_class.o problem_class.o approximation_class.o matrix_class.o list_matrix_class.o dm_class.o op_mat_new_class.o op_mat_mul_class.o op_ass_class.o op_class.o wfm_class.o 
DIST_OBJECTS = distribution_class.o parameters_class.o resources_class.o options_class.o remote_access_class.o dist_const.o dist_types.o dist_data_class.o dist_task_class.o dist_lsnr_class.o dist_common.o parts_mngr_class.o dist_cholesky_blk.o link_list_class.o

TASK_LIBRARY = tl_handle_class.o tl_cpp.o tl_f90.o

PTHREADS = pthreads_c.o pthreads_f90.o

SCHEDULE = schedule.o

clean:
	rm -f *.o *.mod main1 main2 main3 main4 ./test 
	rm distribution_class.f90 op_ass_class.f90  parameters_class.f90 parts_mngr_class.f90 dist_const.f90 dist_types.f90 dist_common.f90 
	rm dist_data_class.f90  dist_lsnr_class.f90  dist_task_class.f90 remote_access_class.f90 wfm_class.f90
	rm a.? b.? viz.txt timing.txt schedule?.dat devents*.txt ./output/*.dat

# Debug programs
test: $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) test.o test.f90
	$(F90.LINK) $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) test.o -o test $(CPP_LIBS)

test_nodes: $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) $(DIST_OBJECTS) test_nodes.o test_nodes.f90
	$(MPIF90.LINK) $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) $(DIST_OBJECTS) test_nodes.o -o test_nodes $(CPP_LIBS)

test_nodes.o: test_nodes.f90
	$(MPIF90.COMPILE) test_nodes.f90

# Using the matrix mult operation
main1: $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) main1.o main1.f90
	$(F90.LINK) $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) main1.o -o main1 $(CPP_LIBS)

# Using the assemble operation
main2: $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) main2.o main2.f90
	$(F90.LINK) $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) main2.o -o main2 $(CPP_LIBS)

# Serial version of assemble to compare runtimes against
main3: highrestimer.o timer.o fp.o fld.o rbf.o const.o out.o pts.o matrix_class.o main3.o main3.f90
	$(F90.LINK) highrestimer.o timer.o fp.o fld.o rbf.o const.o out.o pts.o matrix_class.o main3.o -o main3

# Using the distribution and parallel execution
main4: $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) main4.o main4.f90
	$(MPIF90.LINK) $(PTHREADS) $(TASK_LIBRARY) $(SCHEDULE) $(OBJECTS) $(DIST_OBJECTS) main4.o -o main4 $(CPP_LIBS)

test.o: wfm_class.o test.f90
	$(F90.COMPILE) test.f90

main1.o: wfm_class.o main1.f90
	$(F90.COMPILE) main1.f90

main2.o: wfm_class.o main2.f90
	$(F90.COMPILE) main2.f90

main3.o: main3.f90
	$(F90.COMPILE) main3.f90

main4.o: distribution_class.o main4.f90
	$(MPIF90.COMPILE) main4.f90

distribution_class.o: options_class.o parameters_class.o resources_class.o remote_access_class.o wfm_class.o parts_mngr_class.o distribution_class.f90
	$(MPIF90.COMPILE) distribution_class.f90

distribution_class.f90: distribution_class.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) distribution_class.F90 -o distribution_class.f90


op_ass_class.o: op_ass_class.f90
	$(MPIF90.COMPILE) op_ass_class.f90
op_ass_class.f90:  debug.h op_ass_class.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) op_ass_class.F90 -o op_ass_class.f90 

options_class.o: options_class.f90
	$(F90.COMPILE) options_class.f90

parameters_class.o: parameters_class.f90
	$(F90.COMPILE) parameters_class.f90
parameters_class.f90: debug.h parameters_class.F90
	$(F90.COMPILE) $(PP.FLAGS) parameters_class.F90 -o parameters_class.f90

resources_class.o: resources_class.f90
	$(F90.COMPILE) resources_class.f90

parts_mngr_class.o: parts_mngr_class.f90 wfm_class.o fp.o pts.o
	$(MPIF90.COMPILE)  parts_mngr_class.f90

parts_mngr_class.f90: debug.h  parts_mngr_class.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) parts_mngr_class.F90 -o parts_mngr_class.f90

dist_const.f90: debug.h dist_const.F90
	$(F90.COMPILE) $(PP.FLAGS) dist_const.F90 -o dist_const.f90
dist_const.o: dist_const.f90
	$(F90.COMPILE) dist_const.f90

dist_types.f90: debug.h dist_types.F90
	$(F90.COMPILE) $(PP.FLAGS) dist_types.F90 -o dist_types.f90

dist_types.o: dist_types.f90
	$(F90.COMPILE) dist_types.f90	


link_list_class.o: link_list_class.f90
	$(F90.COMPILE) link_list_class.f90

dist_common.f90: debug.h dist_common.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) dist_common.F90 -o dist_common.f90

dist_common.o: link_list_class.o dist_common.f90
	$(MPIF90.COMPILE) dist_common.f90	

dist_data_class.f90:  debug.h dist_const.o dist_types.o dist_common.o dist_data_class.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) dist_data_class.F90 -o dist_data_class.f90

dist_data_class.o: dist_const.o dist_types.o dist_common.o dist_data_class.f90
	$(MPIF90.COMPILE)  dist_data_class.f90


dist_task_class.f90:  debug.h dist_const.o dist_types.o dist_common.o dist_lsnr_class.o dist_cholesky_blk.o dist_task_class.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) dist_task_class.F90 -o dist_task_class.f90

dist_task_class.o: dist_const.o dist_types.o dist_common.o dist_lsnr_class.o op_ass_class.o wfm_class.o dist_cholesky_blk.o dist_task_class.f90 
	$(MPIF90.COMPILE) dist_task_class.f90

dist_lsnr_class.f90:  debug.h dist_const.o dist_types.o dist_common.o dist_data_class.o dist_lsnr_class.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) dist_lsnr_class.F90 -o dist_lsnr_class.f90

dist_lsnr_class.o: dist_const.o dist_types.o dist_common.o dist_data_class.o dist_lsnr_class.f90
	$(MPIF90.COMPILE) dist_lsnr_class.f90

remote_access_class.f90 :  debug.h dist_const.o dist_types.o dist_data_class.o dist_task_class.o dist_lsnr_class.o dist_common.o remote_access_class.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) remote_access_class.F90 -o remote_access_class.f90


remote_access_class.o : link_list_class.o dist_const.o dist_types.o dist_data_class.o dist_task_class.o dist_lsnr_class.o dist_common.o pts.o dist_cholesky_blk.o remote_access_class.f90 
	$(MPIF90.COMPILE) remote_access_class.f90 


dist_cholesky_blk.o: dist_cholesky_blk.f90
	$(MPIF90.COMPILE) dist_cholesky_blk.f90

dist_cholesky_blk.f90:  debug.h dist_cholesky_blk.F90
	$(MPIF90.COMPILE) $(PP.FLAGS) dist_cholesky_blk.F90 -o dist_cholesky_blk.f90

wfm_class.o: op_class.o wfm_class.f90
	$(F90.COMPILE) wfm_class.f90

wfm_class.f90:  debug.h wfm_class.F90
	$(F90.COMPILE) $(PP.FLAGS) wfm_class.F90 -o wfm_class.f90

op_class.o: op_mat_new_class.o op_mat_mul_class.o op_class.f90
	$(F90.COMPILE) op_class.f90

op_mat_new_class.o: op_mat_new_class.f90
	$(F90.COMPILE) op_mat_new_class.f90

op_mat_mul_class.o: op_mat_mul_class.f90
	$(F90.COMPILE) op_mat_mul_class.f90

op_mat_mul_class.f90:  debug.h op_mat_mul_class.F90
	$(F90.COMPILE) $(PP.FLAGS) op_mat_mul_class.F90 -o op_mat_mul_class.f90


dm_class.o: dm_class.f90
	$(F90.COMPILE) dm_class.f90

matrix_class.o: matrix_class.f90
	$(F90.COMPILE) matrix_class.f90

list_matrix_class.o: tl_handle_class.o matrix_class.o list_matrix_class.f90
	$(F90.COMPILE) list_matrix_class.f90

geometry_class.o: geometry_class.f90
	$(F90.COMPILE) geometry_class.f90

expression_class.o: expression_class.f90
	$(F90.COMPILE) expression_class.f90

problem_class.o: problem_class.f90
	$(F90.COMPILE) problem_class.f90

approximation_class.o: approximation_class.f90
	$(F90.COMPILE) approximation_class.f90
approximation_class.f90:  debug.h approximation_class.F90
	$(F90.COMPILE) $(PP.FLAGS) approximation_class.F90 -o approximation_class.f90


constants.o: constants.f90
	$(F90.COMPILE) constants.f90

rbf.o: rbf/rbf.f90
	$(F90.COMPILE) rbf/rbf.f90

fp.o: rbf/fp.f90
	$(F90.COMPILE) rbf/fp.f90

fld.o: rbf/fld.f90
	$(F90.COMPILE) rbf/fld.f90

eps.o: rbf/eps.f90
	$(F90.COMPILE) rbf/eps.f90

dist.o: rbf/dist.f90
	$(F90.COMPILE) rbf/dist.f90

timer.o: timer.f90
	$(F90.COMPILE) timer.f90

highrestimer.o: rbf/highrestimer.c
	$(C.COMPILE) rbf/highrestimer.c

expr2.o: danhua/expr2.f90
	$(F90.COMPILE) danhua/expr2.f90

fval.o: danhua/fval.f90
	$(F90.COMPILE) danhua/fval.f90

const.o: danhua/const.f90
	$(F90.COMPILE) danhua/const.f90

eqconst.o: danhua/eqconst.f90
	$(F90.COMPILE) danhua/eqconst.f90

out.o: danhua/out.f90
	$(F90.COMPILE) danhua/out.f90

pts.o: danhua/pts.f90
	$(F90.COMPILE) danhua/pts.f90

danhua/pts.f90:  debug.h danhua/pts.F90
	$(F90.COMPILE) $(PP.FLAGS) danhua/pts.F90 -o danhua/pts.f90

##### TASK LIBRARY #####

tl_cpp.o: tl.cpp
	$(CPP.COMPILE) -Itasklib/ tl.cpp -o tl_cpp.o

tl_f90.o: tl_handle_class.o tl.f90
	$(F90.COMPILE) tl.f90 -o tl_f90.o

tl_handle_class.o: tl_handle_class.f90
	$(F90.COMPILE) tl_handle_class.f90

##### PTHREADS #####

pthreads_f90.o: pthreads_c.o pthreads.f90
	$(F90.COMPILE) pthreads.f90 -o pthreads_f90.o

pthreads_c.o: pthreads.c
	$(C.COMPILE) pthreads.c -o pthreads_c.o

##### SCHEDULE #####

schedule.o:
	$(CPP.COMPILE) -DCREATE_SCHEDULE schedule.cpp -o schedule.o
