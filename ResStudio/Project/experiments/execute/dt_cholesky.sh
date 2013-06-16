cholesky() {
    MtdList=(3) 
    CHOL_PART_CNT=1
    CHOLESKY_TEST=1
    Rt=3
    PRINT_TASK_LIST=0
    CHOL_COMM_PIPE=0
    CHOL_LOAD_BAL=0
    NpUnit=1
    NpTot=0
    DrawRecs=0 
    for BlkSz in  3
    do 
	for Npx in  24 #1024  #4096 8192
	do 
	    
	    #--------------------------------------------------------------------- Sequential Run
	    for SEQUENTIAL in  2 
	    do 
		CHOL_CACHEOPT=$[$SEQUENTIAL / 3]
		NtList=(1) ;CHOL_PART=1;NnList=(1);NpList=($Npx)
		Nbx=$[ $Npx  / $BlkSz]
    		NbList=($Nbx)
		SCHEDULE_GRAPH=0
		#run_loop $CholSpeedUp
		cp ${OUT_DIR}/rbfdts.* ${OUT_DIR}/"0-rbfdts".*
		grep ${grep_flags} -i -c "1017 ,madd" $mpioutfile.* 
		grep ${grep_flags} -i "Total Tasks Count" $mpioutfile.* >> ${mpioutfile}_1.txt
		echo "1 node - 1 thread" >> ${TMP_DIR}/err.log
		grep ${grep_flags} -i "sum,error" $mpioutfile.* >> ${TMP_DIR}/err.log
		grep ${grep_flags} -i "trace,error" $mpioutfile.* >> ${TMP_DIR}/err.log
	    done
	    
	    
	    # --------------------------------------------------------------------- Parallel Run 
	    SEQUENTIAL=0
	    for CHOL_CACHEOPT in 0
	    do 
		NpList=($Npx)
		for Nt in 1 #$(seq 2 32)
		do
		    for Node in  2 #$(seq 2 32)
		    do
			Gr=1 #$Node
			Gc=2
			#Nbx=$[ $Npx / $Node / $BlkSz ]
    			NnList=($Node)
    			NtList=($Nt)
			DNb=2
			Nbx=$[ $Npx / $DNb / $BlkSz ]
    			NbList=($Nbx)
			[ $Nbx -gt 32 ] && SCHEDULE_GRAPH=0 || SCHEDULE_GRAPH=0
    			run_loop $CholSpeedUp
			grep ${grep_flags} -i "Total Tasks Count" $mpioutfile.* >> ${mpioutfile}_1.txt
    			if test "$(grep ${grep_flags} -i "timeout" $mpioutfile.*)" != ""
			then 
			    return
    			fi
    			echo "$Node nodes - $Nt threads" >> ${TMP_DIR}/err.log
    			grep ${grep_flags} -i "sum,error" $mpioutfile.* >> ${TMP_DIR}/err.log
    			grep ${grep_flags} -i "trace,error" $mpioutfile.* >> ${TMP_DIR}/err.log
		    done #Node
		    cp ${OUT_DIR}/rbfdts.* "${OUT_DIR}/${Nt}-rbfdts.*"
		    rm ${OUT_DIR}/S*.CB*
		done #Nt
	    done #CacheOpt 
	done #Npx
    done #BlkSize
    #python ${SCRIPTS_DIR}/drawrecs.py $REC_FILE "$REC_FILE.filtered" $CholSpeedUp >> $STATISTICS_FILE
    grep ${grep_flags} -i "SG_TASK" $mpioutfile.* > ${OUT_DIR}/sgtasks.txt
    cat ${TMP_DIR}/err.log
    rm ${TMP_DIR}/err.log # ${OUT_DIR}/rbfdts.*



}