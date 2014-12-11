cmake_minimum_required(VERSION 2.6)
project(DuctTeip)
enable_language(CXX)
include_directories(../../../superglue/include)
find_package(MPI REQUIRED)
set(CMAKE_LINKER mpif90)
set(CMAKE_Fortran_LINK_EXECUTABLE "<CMAKE_LINKER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-lm -lrt -nofor-main")
SET(CMAKE_CXX_COMPILER mpic++)
add_executable(
	ductteip 
	test_task_gen.cpp
)
SET_TARGET_PROPERTIES(ductteip PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(
	ductteip 
	mpi
#	/home/afshin/acml/acmllib/ifort64_fma4/lib/libacml.a
)

install(TARGETS ductteip DESTINATION ${PROJECT_SOURCE_DIR})

set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
include_directories(${MPI_INCLUDE_PATH})
#target_link_libraries(ductteip ${MPI_LIBRARIES})