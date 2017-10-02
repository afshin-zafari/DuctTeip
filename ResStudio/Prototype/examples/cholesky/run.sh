#!/bin/bash 

set -x
date
sleep 1
date
echo "Type:${SLURM_CPU_BIND_TYPE}"
echo "VEBOSE:${SLURM_CPU_BIND_VERBOSE}"
echo "List:${SLURM_CPU_BIND_LIST}"

