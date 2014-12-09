#! /bin/bash

outfile=./temp/test_task_gen.out

# params:   p q N Nb nb
P=4
p=2;q=2
params[1]=" $p $q 24 4 3"   #  6x6   , 2x2
params[2]=" $P 1 48 4 3"   # 12x12  , 4x4
params[3]=" $P 1 96 4 3"   # 24x24  , 8x8
#params[4]=" $P 1 1024 8 4"  # 128x128 , 32x32
params[4]=" $p $q 2048 4 8"  # 512 , 64
params[5]=" $P 1 2048 8 8"  # 256x256 ,  32x32
params[6]=" $P 1 1024 8 4"  # 24x24  , 8x8
params[7]=" $P 1 1024 8 4"  # 24x24  , 8x8
params[8]=" $P 1 125 5 5" # 
params[9]=" $P 1 360 6 5" # 
params[10]=" $P 1 64 4 4"   # 16x16 , 4x4


if test $1 -ne 0
then 
    rm ./temp/a.txt
    rm ./temp/test_task* 
    rm -R ./temp/out*/test_task* 
    rmdir ./temp/out*
fi
if test $2 -ne 0
then 
    make UBUNTUMPI=1 task_gen 
fi
if test $3 -ne 0
then 
    for j in $(seq 1 1)
    do
	for i in $(seq 4 4 )
	do
	    ~/openmpi/bin/mpirun -n $P --output-filename $outfile ./temp/test_task_gen ${params[$i]}
	    echo $i$j>>./temp/a.txt
	    grep -i "error" ${outfile}* >>./temp/a.txt
	    #grep -i "starts" ${outfile}* >>./temp/a.txt
	    if test $4 == 0 
	    then 
		mkdir "./temp/out$i$j"
		mv ${outfile}* "./temp/out$i$j"		
	    fi
	done
    done 
fi
if test $4 -ne 0
then 
    emacs ./temp/test_task_gen.out.1.0
    emacs ./temp/test_task_gen.out.1.1
fi

