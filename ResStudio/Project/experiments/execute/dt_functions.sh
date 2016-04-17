
source ./dt_exp_funcs.sh

create_input_env(){
    create_option_file
    [ $testcase -eq $Cholesky ] && python2 ${SCRIPTS_DIR}/ch.py $Np $Nn $CHOL_PART_CNT 0 0 0 $CHOL_TASKS_FILE  > $CHOL_GTASK
}
create_output_env(){
    PROJECT_FULL_DIR="$HOME/Desktop/ResStudio/Project"
    PROG_FULL_DIR="${PROJECT_FULL_DIR}/build"

    ln -s ${PROJECT_FULL_DIR} proj_link

    PROJECT_DIR=proj_link
    PROG_DIR="${PROJECT_DIR}/build"

    LD_LIBRARY_PATH=${LD_LIBRARY_PATH#${PROG_FULL_DIR}}
    LD_LIBRARY_PATH=${PROG_FULL_DIR}:$LD_LIBRARY_PATH
    export LD_LIBRARY_PATH

    PROG_NAME="${PROG_DIR}/ductteip_app"
    SCRIPTS_DIR="${PROJECT_DIR}/src/scripts"
    FILE_NAME="S$SEQUENTIAL-t$Nt-b$Nb-n$Nn-p$Np-c$CHOL_CACHEOPT"
    OUT_DIR="${PROJECT_DIR}/experiments/records/exp$1"
    GRF_DIR="${PROJECT_DIR}/experiments/reports/exp$1"
    TMP_DIR="${PROJECT_DIR}/experiments/temps/exp$1"
    mkdir $OUT_DIR
    mkdir $GRF_DIR
    mkdir $TMP_DIR
    OUT_FILE="${OUT_DIR}/${FILE_NAME}.txt"
    GRF_FILE="${GRF_DIR}/${FILE_NAME}.png"
    REC_FILE="${OUT_DIR}/rec.txt"
    GRF_ZIP="${OUT_DIR}/grf.tar"
    REC_ZIP="${OUT_DIR}/rec.tar"
    CHOL_GTASK="${OUT_DIR}/chol_gtasks.txt"
    OPTIONS_FILE="./options.txt"
    LOG_FILE="${TMP_DIR}/log.txt"
    STATISTICS_FILE="${TMP_DIR}/allstat.txt"
    mpioutfile="${OUT_DIR}/rbfdts"
    pgm_out_file=" -output-filename $mpioutfile "
    if test "$env" == "$LINUX" 
    then 
	node_file="${PROG_DIR}/nodes_linux"
	mpioutfile="$mpioutfile.1"
	node_tag=" --hostfile ${node_file}  "  
    else
	node_file="${PROG_DIR}/nodes_staff"
	node_tag=" --hostfile ${node_file} -mca plm_rsh_agent rsh " 
    fi
    per_node=" -pernode -npernode 1 -num-boards $NODE_CNT"
    TASK_DIST=$TASK_ONCE

    CHOL_TASKS_FILE="${TMP_DIR}/chtasks.txt"
    SCHED_DIR="${OUT_DIR}"


    echo "options file =$OPTIONS_FILE"

    echo "Seq Nt Nb Np Nn Cx" >> $OUT_FILE
    echo "$SEQUENTIAL $Nt $Nb $Np $Nn $CHOL_CACHEOPT" >> $OUT_FILE
    
    create_input_env $@
}

run_program(){
    export RMA_TIME_OUT=$Rt
    if test "$env" != "$UPPMAX"     # ------------------------------SOLARIS RUN--------------------------------------------------
    then 
	if test "$PURE_MPI" == "0"
	then 
	    if test "$env" == "$SOLARIS"
	    then 
		per_node=" -pernode"   # " -num-boards $NODE_CNT"
	    else # LINUX env
		per_node=" -pernode -num-boards $NODE_CNT"
	    fi
	    mpirun -display-map $per_node $pgm_out_file $node_tag -x LD_LIBRARY_PATH -x RMA_TIME_OUT -np $NODE_CNT ${PROG_NAME} || return 1
	else
	    NODE_CNT=$[$NODE_CNT*$Nt]
	    per_node=" -npernode $Nt -num-boards $NODE_CNT"
	    mpirun -display-map $per_node $pgm_out_file $node_tag -x LD_LIBRARY_PATH -x RMA_TIME_OUT -np $NODE_CNT ${PROG_NAME} || return 1
	fi
    else                            # ------------------------------UPPMAX  RUN--------------------------------------------------	
	if test "$PURE_MPI" == "0"
	then 
            #SBATCH -A p2009014 -p node -N $NODE_CNT -n $NODE_CNT
	    export OMP_NUM_THREADS=$Nt
	    per_node=" -pernode"   # " -num-boards $NODE_CNT"
	    mpirun -display-map $per_node $pgm_out_file  -x NODE_CNT -x RMA_TIME_OUT -np $NODE_CNT ${PROG_NAME} || return 1
	else
	    export OMP_NUM_THREADS=$Nt
	    Nc=$[$NODE_CNT*$Nt]
            #SBATCH -A p2009014 -p node -N $NODE_CNT -n $Nc
	    echo "SBATCH -A p2009014 -p node -N $NODE_CNT -n $Nc"
	    per_node=" -npernode $Nt -num-boards $NODE_CNT"
	    mpirun -display-map $per_node $pgm_out_file  -x NODE_CNT -x RMA_TIME_OUT  -np $Nc  ${PROG_NAME} || return 1
	fi
    fi
    return 0
}
calc_durations() {
    set +x
    for a in $(ls $mpioutfile.*)
    do
	echo $a
	grep ${grep_flags} -i " $E1 0 0 0" $a >> $OUT_FILE
	grep ${grep_flags} -i " $E2 0 0 0" $a >> $OUT_FILE
	echo "$a" >> $STATISTICS_FILE
	TotOnly=1
    done
    set -x
}

extract_timings() {
    if test "$CHOLESKY_TEST" == "1"
    then 
	echo "LB CB"  >> $OUT_FILE
	echo "$CHOL_LOAD_BAL $CHOL_COMM_PIPE"  >> $OUT_FILE
	n=0
	while [ $n -lt $NODE_CNT ] 
	do
    		grep ${grep_flags} -i " $E1 ," $mpioutfile.$n >> $OUT_FILE
    		grep ${grep_flags} -i " $E2 ," $mpioutfile.$n >> $OUT_FILE
	    echo "***$n***"  >> $OUT_FILE
            n=$[$n+1]
	done	
    else
	if test "$PURE_MPI" == "1"
	then 
	    calc_durations
	else
	    n=0
	    while [ $n -lt $NODE_CNT ] 
	    do
		ecnt=0
		while [ "$ecnt" != "2" ]
		do
		    ecnt=$(grep -c ${grep_flags} -i " $E2 0 0 0" $mpioutfile.$n )
		    grep ${grep_flags} -i " $E2 0 0 0" $mpioutfile.$n
		done
		grep ${grep_flags} -i " $E1 0 0 0" $mpioutfile.$n >> $OUT_FILE
		grep ${grep_flags} -i " $E2 0 0 0" $mpioutfile.$n >> $OUT_FILE
		grep ${grep_flags} -i " $E2 0 0 0" $mpioutfile.$n 
		grep ${grep_flags} -i " $E1 0 0 0" $mpioutfile.$n >> devents$n.txt
		grep ${grep_flags} "CHOL_" $mpioutfile.$n >> chol.log
		grep ${grep_flags} "GrpSize" $mpioutfile.$n >>arrays.txt
		grep ${grep_flags}  "\*\*\*" $mpioutfile.$n >>arrays.txt
		echo "Nt=$Nt Nn=$Nn Np=$Np Nb=$Nb" >> D-TEiP.txt
		echo "---------------" >> D-TEiP.txt
		grep ${grep_flags} "asm" $mpioutfile.$n >> D-TEiP.txt
		n=$[$n+1]
	    done
	fi
    fi
}

create_option_file() {
    export NODE_CNT=$Nn
    per_node=" -pernode -npernode 1 -num-boards $NODE_CNT"
    rm $OPTIONS_FILE
    echo "CHUNK_PTS=$Np" >> $OPTIONS_FILE
    echo "BLOCK_CNT=$Nb" >> $OPTIONS_FILE
    echo "BLOCK_CNT2=$Nb2" >> $OPTIONS_FILE
    echo "DATA_READY_LAG=0" >> $OPTIONS_FILE
    echo "TEST_CORRECTNESS=0" >> $OPTIONS_FILE
    echo "SAVE_RESULTS=0" >> $OPTIONS_FILE
    echo "PURE_MPI=$PURE_MPI" >> $OPTIONS_FILE
    echo "BROADCAST_MTD=$BCAST_MTD" >> $OPTIONS_FILE
    echo "SEQUENTIAL=$SEQUENTIAL" >> $OPTIONS_FILE
    echo "TASK_DISTRIBUTION=$TASK_DIST" >> $OPTIONS_FILE
    echo "SEQ_CHUNK=$SEQ_CHUNK" >> $OPTIONS_FILE
    echo "LOAD_BALANCE=$LBM_METHOD" >> $OPTIONS_FILE
    echo "GROUP_SIZE=$GROUP_SIZE" >> $OPTIONS_FILE
    echo "CHOLESKY=$CHOLESKY_TEST" >> $OPTIONS_FILE
    echo "PART_CNT=$PART_CNT" >> $OPTIONS_FILE
    echo "CHOL_PART_CNT=$CHOL_PART_CNT" >> $OPTIONS_FILE
    echo "CHOL_CACHEOPT=$CHOL_CACHEOPT" >> $OPTIONS_FILE
    echo "NODE_CAPACITY=$NODE_CAPACITY" >> $OPTIONS_FILE
    echo "TIME_OUT=$Rt" >> $OPTIONS_FILE
    echo "CHOL_TASKS_FILE=$CHOL_TASKS_FILE" >> $OPTIONS_FILE
    echo "DIST_BLK_CNT=$DNb" >> $OPTIONS_FILE
    echo "CHOL_GRP_ROWS=$Gr" >> $OPTIONS_FILE
    echo "CHOL_GRP_COLS=$Gc" >> $OPTIONS_FILE
    echo "SCHED_DIR=$SCHED_DIR" >> $OPTIONS_FILE
    echo "export RMA_TIME_OUT=$Rt" >> $OPTIONS_FILE
    echo "export NODE_CNT=$Nn" >> $OPTIONS_FILE
    echo "export TLIB_THREAD_COUNT=$Nt" >> $OPTIONS_FILE
    if test "$Mtd" == "3"
    then
        echo "NODE_AUTONOMY=1" >> $OPTIONS_FILE
    else
        echo "NODE_AUTONOMY=0" >> $OPTIONS_FILE
        if test "$Mtd" == "2"
        then
            echo "BROADCAST_MTD=1" >> $OPTIONS_FILE
        else
            echo "BROADCAST_MTD=0" >> $OPTIONS_FILE
        fi
    fi
    if test "$CHOLESKY_TEST" == "1"
    then
        echo "IMPORT_TASKS=1" >> $OPTIONS_FILE
    else
        echo "IMPORT_TASKS=0" >> $OPTIONS_FILE
    fi
}
run_loop() {
    for Mtd in "${MtdList[@]}"
    do
	for Nt in "${NtList[@]}"
	do
	    for Nb in "${NbList[@]}"
	    do
		for Npt in "${NpList[@]}"
		do
		    for Nn in "${NnList[@]}"
		    do
			Np=$[$Npt*$NpUnit]
			[ $PURE_MPI -eq 1 ]  && [ $NpTot -eq 1 ] && Np=$[$Np/($Nn*$Nt)]
			[ $PURE_MPI -eq 0 ]  && [ $NpTot -eq 1 ] && Np=$[$Np/$Nn]
			create_output_env $1
			run_prog_prologue
			run_program || continue
			run_prog_epilogue
		    done #Nn Loop
		done #Np Loop
	    done #Nb Loop
	done #Nt Loop
    done #Mtd Loop
    if test "$env" != "$UPPMAX"
	then 
	if test "$DrawRecs" == "1"
	then
	    python ${SCRIPTS_DIR}/drawrecs.py $REC_FILE "$REC_FILE.filtered" $1 >> $STATISTICS_FILE
	fi
    fi
}

run_test_cases()
{
    for testcase in $@
    do
	case $testcase in 
	    $SpeedUp)     speedup        ;;
            $PureMPIComp) pure_MPI       ;;
	    $Cholesky)    cholesky       ;;
	    $WeakScaling) weak_scaleup   ;;
	    $ScaleUp)     strong_scaleup ;;
	    $PartPerNode) partpernode    ;;
        esac
    done
}


clean_output_env()
{
    rm $OUT_DIR/S*.txt*
}

cholesky_epilogue()
{
    CHOL_FILE="$OUT_FILE.LB$CHOL_LOAD_BAL.CB$CHOL_COMM_PIPE"
    TMP_CHOL="${TMP_DIR}/tmpchol.txt"
    cp $OUT_FILE  $CHOL_FILE
    [ $SCHEDULE_GRAPH -ne 0 ] && python ${SCRIPTS_DIR}/drawschedn.py $Nn $Nt $OUT_FILE $SCHED_DIR
    grep "SUBTASK" $mpioutfile.* > ${TMP_CHOL}.1
    echo "Seq=$SEQUENTIAL,Nt=$Nt,Nn=$Nn,Np=$Np" >> ${OUT_DIR}/subtasks.txt
    python2 ${SCRIPTS_DIR}/drawexp.py $CHOL_FILE $REC_FILE $PURE_MPI $PART_CNT  $CHOL_CACHEOPT ${TMP_CHOL}.1 | grep "avg" >> ${OUT_DIR}/subtasks.txt
    rm $OUT_FILE

}
run_prog_prologue()
{
    rm $mpioutfile.* "$OUT_DIR/S*.CB*" "$OUT_DIR/schedule?.dat"
}
run_prog_epilogue()
{
    extract_timings
#    [ $PURE_MPI -eq 0 ] && [ $Nt -gt 5 ] && [ $Nn -gt 5 ] && python ${SCRIPTS_DIR}/drawschedn.py $Nn $Nt $OUT_FILE $SCHED_DIR
#    python ${SCRIPTS_DIR}/drawschedn.py $Nn $Nt $OUT_FILE $SCHED_DIR
    if test "$CHOLESKY_TEST" == "1" 
    then
	cholesky_epilogue
    else
	echo "$PURE_MPI $Np $Nn $Nt" >> $LOG_FILE
	grep ${grep_flags} -c "TimeOut #" $mpioutfile.* >> $LOG_FILE
	python2 ${SCRIPTS_DIR}/drawexp.py $OUT_FILE $REC_FILE $PURE_MPI $PART_CNT $CHOL_CACHEOPT $mpioutfile.*
	tar -rf $REC_ZIP $OUT_FILE 
	tar -rf $GRF_ZIP $GRF_FILE 
	rm ${OUT_FILE}  #${GRF_FILE} 
    fi

}