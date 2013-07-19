rm ./temp/test9
rm test*.out.*
make UBUNTUMPI=1 prog_list=9 all
~/openmpi/bin/mpirun -n 4 --output-filename test9.out ./temp/test9 4 4 
#~/openmpi/bin/mpirun -n 4 -tag-output ./temp/test9 4 4 > a.txt
emacs test9.out.1.0 &

