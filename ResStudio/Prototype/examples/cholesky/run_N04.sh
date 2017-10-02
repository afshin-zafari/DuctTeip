#!/bin/bash

#SBATCH -A snic2017-7-18
#SBATCH -p node 
#SBATCH -N 4
#SBATCH -n 80
#SBATCH -t 00:15:00
#SBATCH -J Cholesky_N04
#SBATCH -o Cholesky-N04-%j

DLB=0;ps=2000;to=2000;

ipn=1;nt=20

P=4;p=2;q=2;
B=6;b=8;

N=48000

source ./main.sh



