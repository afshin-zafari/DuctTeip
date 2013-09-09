make 
Np=6
for i in 10 50 100 200 300 400 500
do
    ./temp/test1 $i $Np > temp.txt
    grep -i "@" temp.txt > a.txt
    grep -i "#" temp.txt > b.txt
    python taskgenplot.py >> c.txt
    cp b.txt stat_nb-$i.txt
done
python taskgen_compare.py 
cp c.txt stat_compare_Np-$Np.txt
mkdir NP_$Np
mv stat*.txt NP_$Np
mv task*.png NP_$Np
rm ?.txt

echo "finished"
