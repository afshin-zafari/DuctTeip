make UBUNTU=1 all
Np=4
for i in 5 6 7 8 9 10
do
    ./temp/test2 $i $Np > temp.txt
    grep -i "@insert" temp.txt > b.txt
    sort b.txt > sort_$i.txt
done
rm ?.txt

echo "finished"
