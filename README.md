# DuctTeip
DuctTeip is a framework written in C++ for task based parallel programming in distributed environments. It uses MPI for communication and inter node parallelism, and SuperGlue framework for shared memory multi-core parallelism.
SuperGlue is another C++ task based parallel programming framework that can be found in [SuperGlue git repository](https://github.com/tillenius/superglue.git).

# Installation and Build
- Clone both the DuctTeip and SuperGlue repository to local paths.
- Edit the `Makefile` in the Prototype folder such that the SUPERGLUE_DIR points to the local path where the SuperGlue framework is cloned.
- Ensure that MPI environment settings are available. (Load the module, if you compile in cluster environments)
- Run `make` where the Makkefile exists. Two debug and release versions of DuctTeip library are produced at ./lib and ./lib/release directories, corrspondingly.

# Build the examples
To build and run the examples that exist in the sub-directories of the cloned repository:
- Edit the `Makefile` such that the DUCTTEIP_DIR and SUPERGLUE_DIR point to the places where these frameworks are copied.
- Run `make` to get the compiled output
# Runtime command line flags used by DuctTeip:
Flags (prepended with a '-') | Meaning
---------------------------- | --------
 P  |  Total number of processors
 p |     No. of processor in rows, in grid topology
 q|      No. of processor in columns, in grid topology
 M| No. of elements of input matrix in row
 N| No. of elements of input matrix in columns
 B| No. of blocks at DuctTeip level 
 b| No. of blocks at SuperGlue level
 t| number of threads at each computing node
 DLB | Use Dynamic Load Balancing for task execution at DuctTeip level
