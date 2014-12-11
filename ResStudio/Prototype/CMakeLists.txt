cmake_minimum_required(VERSION 2.6)
project(DuctTeip)
enable_language(CXX)
include_directories(../../../superglue/include)
add_executable(
	ductteip 
	basic.hpp
	chol_fact.hpp
	config.hpp
	context.hpp
	data.hpp
	dt_log.hpp
	dt_thread_manager.hpp
	engine.hpp
	glb_context.hpp
	hostpolicy.hpp
	listener.hpp
	mailbox.hpp
	memory_manager.hpp
	mpi_comm.hpp
	network.hpp
	partition.hpp
	procgrid.hpp
	simulate_comm.hpp
	task.hpp
	test_task_gen.cpp
)
SET_TARGET_PROPERTIES(ductteip PROPERTIES LINKER_LANGUAGE FORTRAN)
SET(CMAKE_FORTRAN_LINK_EXECUTABLE mpif90)
SET(CMAKE_CXX_COMPILER mpic++)