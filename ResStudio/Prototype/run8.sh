rm ./temp/test8
make UBUNTUMPI=1 prog_list=8 all
~/openmpi/bin/mpirun -n 4 --output-filename test8.out ./temp/test8 4 4 
#~/openmpi/bin/mpirun -n 4 -tag-output ./temp/test8 4 4 > a.txt

