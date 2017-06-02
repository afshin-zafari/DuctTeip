#!/bin/bash
P=2;p=2;q=1
nt=2;
T=10
ipn=2
set -x
params="-M 10 2 5 -N 10 2 5 -P $p -q $q -p $p -t $nt -T $T"
outfile=out.txt
mpirun -n 2 -ppn $ipn -l --outfile-pattern $outfile --errfile-pattern $outfile ./bin/fmmbase $params
rm *file*.txt
grep "\[0\]" $outfile  > 0.txt
grep "\[1\]" $outfile > 1.txt
sed -i -e 's/\[0\]//g' 0.txt 
sed -i -e 's/\[1\]//g' 1.txt 

rm $outfile 
