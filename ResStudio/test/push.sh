#!/bin/bash

rm -R test_logfile
rm *.txt
make clean 
git add . 
git commit -m "$1"
