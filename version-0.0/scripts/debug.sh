./main2
cp output/output.dat output/output1.dat
./main2
cp output/output.dat output/output2.dat
./main2
cp output/output.dat output/output3.dat
./main2
cp output/output.dat output/output4.dat
./main2
cp output/output.dat output/output5.dat
diff -q output/output.dat output/output1.dat
diff -q output/output.dat output/output2.dat
diff -q output/output.dat output/output3.dat
diff -q output/output.dat output/output4.dat
diff -q output/output.dat output/output5.dat