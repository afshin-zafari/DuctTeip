#!/bin/bash

#SBATCH -A snic2017-7-18
#SBATCH -p node
#SBATCH -N 2
#SBATCH -n 40
#SBATCH -t 00:55:00
#SBATCH -J Simulation
#SBATCH -o SimOut-%j

DLB=0;
comp="intel"

if [ $comp == "gcc" ] ; then 
    module load gcc openmpi
else
    module load intel intelmpi
fi

set -x 

nt=20
to=10000
ps=1000
ipn=1

sim_N=100000
sim_P=9
sim_file="sim_pars_${sim_N}_${sim_P}.sh"

python sim_pars.py ${sim_N} ${sim_P} > ${sim_file}


sim_dir=Sim_N${sim_N}-P${sim_P}
mkdir -p ${sim_dir}


source ./${sim_file}

function create_pars {
  set_pars $1
  params="-P $P -p $p -q $q -N $N $B $b -I $ipn -T $to -t $nt -S $ps "
  param_str="-P$P-p$p-q$q-N$N-B$B-b$b"
  if [ "x$DLB" == "x0" ] ; then 
      params_long="--num-proc $P --proc-grid-row $p --proc-grid-col $q --data-rows $N $B $b --ipn $ipn --timeout $to --num-threads $nt --poll-sleep $ps --simulation  "
  else
      params_long="--num-proc $P --proc-grid-row $p --proc-grid-col $q --data-rows $N $B $b --ipn $ipn --timeout $to --num-threads $nt --poll-sleep $ps --dlb --silent-dur 700 --dlb-threshold 10 --dlb-smart "
  fi

  outfile=../${sim_dir}/sim_result_${param_str}_${SLURM_JOB_ID}
}

cd bin
pwd
app=cholesky
date
if [ $comp == "gcc" ] ; then 
    mpiexec -n $P --cpus-per-proc 2 -npernode 10 -bind-to core --output-filename ${outfile} ./cholesky ${params_long}
else
    for sim_no in $(seq 8 8) # $sim_cnt)
    do
      create_pars $sim_no
      srun -n $P -l --output=$outfile $app ${params_long} || true
    done
fi
date
echo "Finished."
grep -i "error" ${outfile}.*
rm *file*.txt

cd ..
