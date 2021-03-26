#!/bin/bash
make
ipn=2;P=2;p=2;q=1;nt=2;DLB=0;ps=2000;B=10;b=2;to=10000;N=100
params_long="--num-proc $P --proc-grid-row $p --proc-grid-col $q --data-rows $N $B $b --ipn $ipn --timeout $to --num-threads $nt --poll-sleep $ps --pure-mpi"
MPIRUN=/home/afshin/opt/openmpi/bin/mpirun
outfile=test_logfile
echo "$MPIRUN -n $P --output-filename ${outfile} ./bin/test_gcc ${params_long}"
$MPIRUN -n $P --output-filename ${outfile} ./bin/test_gcc ${params_long}
rm *.txt *.dat