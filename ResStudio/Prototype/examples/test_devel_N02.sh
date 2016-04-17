#!/bin/bash

#SBATCH -A p2009014
#SBATCH -p devel
#SBATCH -N 2
#SBATCH -n 32
#SBATCH -t 00:01:10
#SBATCH -J TestLibrary
#SBATCH -o Develop-LibTest-N02-%j

module unload intel openmpi
module load intel openmpi
ompi_info | grep -i thread
set -x 

DLB=0

P=2
p=2
q=1
ipn=1
nt=16

N=15120
ps=2500 # poll-sleep in usec
B=7
b=6
to=120
outfile=devel_test_N02.out

params="-P $P -p $p -q $q -N $N $B $b -I $ipn -T $to -t $nt -S $ps"
params_long="--num-proc $P --proc-grid-row $p --proc-grid-col $q --data-rows $N $B $b --ipn $ipn --timeout $to --num-threads $nt --poll-sleep $ps "


ACML_DIR=/home/afshin/acml/acmllib/ifort64_fma4
ACML_LIB=$ACML_DIR/lib/libacml.a
alloc_mem="--mca mpi_show_mpi_alloc_mem_leaks 1 "
handle_leaks="--mca mpi_show_handle_leaks 1 "
show_params=" --mca mpi_show_mca_params 1 "
full_help=" --mca orte_base_help_aggregate 0 "
free_handles=" --mca mpi_no_free_handles 1"
mca_params="${show_params} ${handle_leaks} ${alloc_mem} ${full_help} ${free_handles}"
mpi_params=" --map-by ppr:$ipn:node --output-filename $outfile "
mpi_params=" -npernode $ipn --output-filename $outfile"

export LD_LIBRARY_PATH=$ACML_DIR/lib:$LD_LIBRARY_PATH

cd bin
#catchsegv 
mpirun -n $P ${mpi_params} cholesky $params_long
echo "Finished."
grep -i "error" ${outfile}.*
grep "\[\*" ${outfile}.*
cd ..
