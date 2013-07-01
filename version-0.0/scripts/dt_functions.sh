create_output_env(){
    FILE_NAME="S$SEQUENTIAL-t$Nt-b$Nb-n$Nn-p$Np-c$CHOL_CACHEOPT"
    OUT_DIR="output/Drawing$1"
    mkdir $OUT_DIR
    OUT_FILE="$OUT_DIR/${FILE_NAME}.txt"
    GRF_FILE="$OUT_DIR/${FILE_NAME}.png"
    REC_FILE="$OUT_DIR/rec.txt"
    GRF_ZIP="$OUT_DIR/grf.tar"
    REC_ZIP="$OUT_DIR/rec.tar"
    CHOL_GTASK="$OUT_DIR/chol_gtasks.txt"
    echo "Seq Nt Nb Np Nn Cx" >> $OUT_FILE
    echo "$SEQUENTIAL $Nt $Nb $Np $Nn $CHOL_CACHEOPT" >> $OUT_FILE
}
run_program(){
    export RMA_TIME_OUT=$Rt
    if test "$env" == "$SOLARIS"     # ------------------------------SOLARIS RUN--------------------------------------------------
    then 
	if test "$PURE_MPI" == "0"
	then 
	    per_node=" -pernode"   # " -num-boards $NODE_CNT"
	    mpirun -display-map $per_node $pgm_out_file $node_file -x NODE_CNT -x RMA_TIME_OUT -np $NODE_CNT main4 || return 1
	else
	    NODE_CNT=$[$NODE_CNT*$Nt]
	    per_node=" -npernode $Nt -num-boards $NODE_CNT"
	mpirun -display-map $per_node $pgm_out_file $node_file -x NODE_CNT -x RMA_TIME_OUT -np $NODE_CNT main4 || return 1
	fi
    else                            # ------------------------------UPPMAX  RUN--------------------------------------------------	
	if test "$PURE_MPI" == "0"
	then 
            #SBATCH -A p2009014 -p node -N $NODE_CNT -n $NODE_CNT
	    export OMP_NUM_THREADS=$Nt
	    per_node=" -pernode"   # " -num-boards $NODE_CNT"
	    mpirun -display-map $per_node $pgm_out_file  -x NODE_CNT -x RMA_TIME_OUT -np $NODE_CNT main4 || return 1
	else
	    export OMP_NUM_THREADS=$Nt
	    Nc=$[$NODE_CNT*$Nt]
            #SBATCH -A p2009014 -p node -N $NODE_CNT -n $Nc
	    echo "SBATCH -A p2009014 -p node -N $NODE_CNT -n $Nc"
	    per_node=" -npernode $Nt -num-boards $NODE_CNT"
	    mpirun -display-map $per_node $pgm_out_file  -x NODE_CNT -x RMA_TIME_OUT  -np $Nc  main4 || return 1
	fi
    fi
    return 0
}
hardcode_timings(){
    echo "TIMING $T1 $E1 " >> $OUT_FILE
    echo "TIMING $T2 $E1 " >> $OUT_FILE
    echo "TIMING $T1 $E2 " >> $OUT_FILE
    echo "TIMING $T3 $E2 " >> $OUT_FILE
    T1=$[$T1+700000]
    T2=$[$T2+600000]
    T3=$[$T3+500000]
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
    		grep ${grep_flags} -i ", $E1 ," $mpioutfile.$n >> $OUT_FILE
    		grep ${grep_flags} -i ", $E2 ," $mpioutfile.$n >> $OUT_FILE
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
			echo "NodeCount = $Nn"
			[ $PURE_MPI -eq 1 ]  && [ $NpTot -eq 1 ] && Np=$[$Np/($Nn*$Nt)]
			[ $PURE_MPI -eq 0 ]  && [ $NpTot -eq 1 ] && Np=$[$Np/$Nn]
			create_option_file
			create_output_env $1
			[ $CHOLESKY_TEST -eq 1 ] && python2 ch.py $Np $Nn $CHOL_PART_CNT $CHOL_LOAD_BAL $CHOL_COMM_PIPE $PRINT_TASK_LIST  > $CHOL_GTASK
			Nx=$NxMin
			while [ $Nx -lt $NxMax ]
			do
			    Nx=$[$Nx+1]
			    [ "$Nx" != "$SNx" ]  && [ "$SNx" != "" ] && continue
			    if test "$exec" == "1" 
			    then 
				rm $mpioutfile.* devents?.txt "$OUT_DIR/M*.txt" schedule?.dat
				run_program || continue
				extract_timings
				if test "$CHOLESKY_TEST" == "1" 
				then
				    CHOL_FILE="$OUT_FILE.LB$CHOL_LOAD_BAL.CB$CHOL_COMM_PIPE"
				    cp $OUT_FILE  $CHOL_FILE
				    [ $PURE_MPI -eq 0 ] && python drawschedn.py $Nn $Nt $OUT_FILE
				    grep "SUBTASK" $mpioutfile.* > az.txt
				    echo "Seq=$SEQUENTIAL,Nt=$Nt,Nn=$Nn" >>subtasks.txt
				    python2 drawexp.py $CHOL_FILE $REC_FILE $PURE_MPI $PART_CNT  $CHOL_CACHEOPT az.txt | grep "avg" >> subtasks.txt
				    rm $OUT_FILE
				else
#				    [ $PURE_MPI -eq 0 ] && [ $Nt -gt 5 ] && [ $Nn -gt 5 ] && python drawschedn.py $Nn $Nt $OUT_FILE
#				    python drawschedn.py $Nn $Nt $OUT_FILE
				    echo "$PURE_MPI $Np $Nn $Nt" >> $LOG_FILE
				    grep ${grep_flags} -c "TimeOut #" $mpioutfile.* >> $LOG_FILE
				    python2 drawexp.py $OUT_FILE $REC_FILE $PURE_MPI $PART_CNT $CHOL_CACHEOPT $mpioutfile.*
				    tar -rf $REC_ZIP $OUT_FILE 
				    tar -rf $GRF_ZIP $GRF_FILE 
				    rm ${OUT_FILE}  #${GRF_FILE} 
				fi
			    else
				hardcode_timings
			    fi
			done #Nx Loop
		    done #Nn Loop
		done #Np Loop
	    done #Nb Loop
	done #Nt Loop
    done #Mtd Loop
    if test "$env" != "$UPPMAX"
	then 
	if test "$DrawRecs" == "1"
	then
	    python drawrecs.py $REC_FILE "$REC_FILE.filtered" $1 >> $STATISTICS_FILE
	fi
    fi
}
pure_MPI(){
    BCAST_MTD=$BCAST_PIPE
    GROUP_SIZE=0
    PART_CNT=1
    NpTot=1
    Rt=300
    NpUnit=8192
    DrawRecs=0
    for xNx in $(seq 1 5)
    do
	for xNt in 1 2 4 8 3 5 6 7
	do
	    for xNn in 1 2 4 8
	    do
		xNb=$[32 / $xNn  ]
		MtdList=(3) ; NtList=( $[$xNt + 0 ]  ) ; NpList=(1); NnList=($xNn);NbList=(1)
		PURE_MPI=1  ; run_loop $PureMPIComp
		NbList=($xNb)
		NtList=($xNt)
		Nb2=$xNb  
		PURE_MPI=0  ; run_loop $PureMPIComp
	    done
	done
    done
    python drawrecs.py $REC_FILE "$REC_FILE.filtered" $PureMPIComp >> $STATISTICS_FILE
    
    PURE_MPI=0  	    
    NpTot=0
}
partpernode()
{
    NODE_CAPACITY=10
    SEQUENTIAL=0
    LBM_METHOD=$LBM_NONE #TASK_STEALING
    GROUP_SIZE=0
    PART_CNT=4
    NpUnit=1
    NpTot=1
    PURE_MPI=0
    Rt=3
    MtdList=(3) ; NtList=(8) ; NpList=(1000); NnList=(1); NbList=(3)
    run_loop $PartPerNode
    grep ${grep_flags} "GrpSize" rbfd*
    grep  ${grep_flags} "\*\*\*" rbfd*
    for k in 1 2 3 4 5 6 7 8
    do
	grep ${grep_flags} -c " 1022 $k " rbfd*
    done
}
weak_scaleup(){
    BCAST_MTD=$BCAST_PIPE
    GROUP_SIZE=0
    PART_CNT=1
    SEQUENTIAL=0
    Nb2=1
    Rt=300
    NpUnit=1
    NpTot=0
    WorkPerCore=1000000
    MtdList=(3) ;  NbList=(8)
    DrawRecs=0
    for xNn in  $(seq 1 8 )
    do
	for xNt in $(seq 1 7) 
	do
	    xNc=$[$xNn*$xNt]
	    xNp=$(echo "scale=2; sqrt($xNc * $WorkPerCore)" | bc -l)
	    xNpn=$(echo "scale=2; ($xNp/  $xNn ) *10/10" | bc -l)
	    xNm=$(echo "scale=2; $xNpn * $xNpn * $xNn" | bc -l ) 
	    xNq=$(echo "scale=0;$xNm  / $xNt" | bc -l )
	    xNq=$(echo "scale=0;$xNp * $xNp / $xNc" | bc -l)
	    echo "---------------------> $xNq $xNn"
	    xNpn=$(echo "scale=0; ($xNpn ) *10/10" | bc -l)
	    NpList=($xNpn)
	    NnList=($xNn)
	    NtList=($xNt)		    
	    run_loop $WeakScaling 
	done
    done
    python drawrecs.py $REC_FILE "$REC_FILE.filtered" $1 >> $STATISTICS_FILE
    DrawRecs=1
}
strong_scaleup(){
    BCAST_MTD=$BCAST_PIPE
    GROUP_SIZE=0
    PART_CNT=1
    NpUnit=1
    NpTot=0
    Rt=300
    Nb2=4
    MtdList=(3) ;  NpList=(2000);  NbList=($Nb2)
    NnList=( 1 2 3 4 5 6 7 8 )  
    NtList=(  2 )  
    run_loop $ScaleUp		
    NtList=(  4 )  
    run_loop $ScaleUp		
    NtList=(  6 )  
    run_loop $ScaleUp		
    NtList=(  7 )  
    run_loop $ScaleUp		  
}
speedup(){
    BCAST_MTD=$BCAST_PIPE
    Rt=30
    GROUP_SIZE=0
    PART_CNT=1
    NpUnit=1
    NpTot=1
    SEQUENTIAL=0
    SEQ_CHUNK=10000
    Nb2=1
    xNp=8000
    DrawRecs=0
    MtdList=(3) ; NtList=(1) ; NpList=($xNp); NnList=(1 ); NbList=(1)
#	    run_loop $SpeedUp
    SEQUENTIAL=0
    PART_CNT=1
    xNb=4
    Nb2=$xNb
    MtdList=(3) ; NtList=(2 3 4 5 6 7)  ; NpList=($xNp); NnList=(3 4 5 6 7 8); NbList=($xNb)
#	    MtdList=(3) ; NtList=(1)  ; NpList=($xNp); NnList=(2); NbList=($xNb)
    run_loop $SpeedUp
}
zcreate_figures()
{
    echo "Nothing!!!"
    for GROUP in $@
    do
	if test "$GROUP" == "$SpeedUp" 
	then    
	    speedup
	fi
    done
}
create_figures()
{
    for GROUP in $@
    do
	case $GROUP in 
	    $SpeedUp) echo "speedup" ;;
            $PureMPIComp) pure_MPI  ;;
	    $Cholesky)  cholesky    ;;
	    $WeakScaling) weak_scaleup ;;
	    $ScaleUp)  strong_scaleup ;;
	    $PartPerNode) partpernode ;;
        esac
    done
}
