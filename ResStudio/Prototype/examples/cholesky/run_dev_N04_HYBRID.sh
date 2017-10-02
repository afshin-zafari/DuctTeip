#!/bin/bash

#SBATCH -A snic2017-7-18
#SBATCH -p node
#SBATCH -N 4
#SBATCH -n 80
#SBATCH -t 00:15:00
#SBATCH -J Hybrid_TestCholesky_N04
#SBATCH -o Hybrid_Cholesky-Test-N04-%j

comp="intel"

if [ $comp == "gcc" ] ; then 
    module load gcc openmpi
else
    module load intel intelmpi
fi

set -x 


ipn=1;P=4;p=2;q=2;nt=20;DLB=0;ps=2000;B=20;b=3;to=1000;N=49920

OPENBLAS_NUM_THREADS=1


params="-P $P -p $p -q $q -N $N $B $b -I $ipn -T $to -t $nt -S $ps "
if [ "x$DLB" == "x0" ] ; then 
    params_long="--num-proc $P --proc-grid-row $p --proc-grid-col $q --data-rows $N $B $b --ipn $ipn --timeout $to --num-threads $nt --poll-sleep $ps "
else
    params_long="--num-proc $P --proc-grid-row $p --proc-grid-col $q --data-rows $N $B $b --ipn $ipn --timeout $to --num-threads $nt --poll-sleep $ps --dlb --silent-dur 700 --dlb-threshold 10 --dlb-smart "
fi

outfile=test_devel_$DLB_${SLURM_JOB_ID}

cd bin

mkdir -p PURE_HYBRID_${SLURM_JOB_ID}
cd PURE_HYBRID_${SLURM_JOB_ID}

app=../cholesky

date
if [ $comp == "gcc" ] ; then 
    mpiexec -n $P --cpus-per-proc 2 -npernode 10 -bind-to core --output-filename ${outfile} ./cholesky ${params_long}
else
    srun  -n $P -l --output=HYBRID_$outfile $app ${params_long}   

fi
date

echo "Finished."

grep -i "error" ${outfile}.*
rm *file*.txt
cd ../..
