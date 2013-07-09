rm ./temp/test8
make KALKYLMPI=1 prog_list=8 all
mpirun -n 4 --output-filename test8.out ./temp/test8 4 4 
#mpirun -n 4 -tag-output ./temp/test8 4 4 > a.txt

