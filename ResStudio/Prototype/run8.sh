make UBUNTUMPI=1 prog_list=8 all
~/openmpi/bin/mpirun -n 4 --output-filename test8.out ./temp/test8 4 4 
