#!/bin/bash

#SBATCH -A p2009014
#SBATCH -p devel
#SBATCH -N 2
#SBATCH -n 32
#SBATCH -t 00:01:30
#SBATCH -J TestDLB
#SBATCH -o Develop-DLBTest-N02-%j

#module unload intel openmpi
#module load intel openmpi

set -x 

#ipn=2;P=2;p=2;q=1;nt=2;DLB=0;ps=2000;B=7;b=6;to=30;N=2520
#ipn=2;P=2;p=2;q=1;nt=2;DLB=1;ps=2000;B=7 ;b=6;to=30;N=2520
#ipn=5;P=5;p=5;q=1;nt=2;DLB=1;ps=2000;B=16;b=4;to=40;N=4608
# ipn=3;P=5;p=1;q=5;nt=16;DLB=1;ps=2000;B=10;b=2;to=50;N=7200
 ipn=2;P=2;p=1;q=2;nt=2;DLB=0;ps=2000;B=3;b=3;to=10;N=72





params="-P $P -p $p -q $q -N $N $B $b -I $ipn -T $to -t $nt -S $ps "
if [ "x$DLB" == "x0" ] ; then 
    params_long="--num-proc $P --proc-grid-row $p --proc-grid-col $q --data-rows $N $B $b --ipn $ipn --timeout $to --num-threads $nt --poll-sleep $ps "
else
    params_long="--num-proc $P --proc-grid-row $p --proc-grid-col $q --data-rows $N $B $b --ipn $ipn --timeout $to --num-threads $nt --poll-sleep $ps --dlb --silent-dur 700 --dlb-threshold 10 --dlb-smart "
fi
outfile=test_devel_$DLB
ACML_DIR=/home/afshin/acml/acmllib/ifort64_fma4
ACML_LIB=$ACML_DIR/lib/libacml.a
   alloc_mem="--mca mpi_show_mpi_alloc_mem_leaks 0 "
handle_leaks="--mca mpi_show_handle_leaks 1 "
 show_params=" --mca mpi_show_mca_params 0 "
 full_help=" --mca orte_base_help_aggregate 0 "
free_handles=" --mca mpi_no_free_handles 1"
  mca_params="${show_params} ${handle_leaks} ${alloc_mem} ${full_help} ${free_handles}"
  mpi_params=" -npernode $ipn --output-filename $outfile"
  mpi_params="--map-by ppr:$ipn:node --output-filename $outfile"

export LD_LIBRARY_PATH=$ACML_DIR/lib:$LD_LIBRARY_PATH
build(){
    cd ..
    make
    cd examples
    make clean 
    make
}
create_example(){
    make clean
    make
}
if [ "x$1x" == "xcleanx" ] ; then 
    cd ..
    make clean 
    cd examples
    build
fi

if [ "x$1x" == "xbuildx" ] ; then 
  build
fi
if [ "x$1x" == "xtestx" ] ; then 
  create_example
fi
cd bin
mpirun -n $P ${mpi_params} cholesky ${params_long}
echo "Finished."
grep -i "error" ${outfile}.*
rm *log*.txt
cd ..
