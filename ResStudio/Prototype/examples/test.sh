#!/bin/bash

#SBATCH -A p2009014
#SBATCH -p devel
#SBATCH -N 1
#SBATCH -n 16
#SBATCH -t 00:00:10
#SBATCH -J TestLib
#SBATCH -o DTLibTest

bash -x ./run
