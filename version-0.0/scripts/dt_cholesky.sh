cholesky() {
    MtdList=(3) 
    CHOL_PART_CNT=1
    CHOLESKY_TEST=1
    Rt=3
    PRINT_TASK_LIST=0
    CHOL_COMM_PIPE=0
    CHOL_LOAD_BAL=0
    NpUnit=1
    NpTot=1
    DrawRecs=0
    for BlkSz in 16
    do 
	for Npx in  512 #1024  #4096 8192
	do 
	    rm err.log rbfdts.*
	    
	    #--------------------------------------------------------------------- Sequential Run
	    for SEQUENTIAL in  2  3
	    do 
		CHOL_CACHEOPT=$[$SEQUENTIAL / 3]
		NtList=(1) ;CHOL_PART=1;NnList=(1);NpList=($Npx)
		Nbx=$[ $Npx  / $BlkSz]
    		NbList=($Nbx)
		run_loop $CholSpeedUp
		grep -i -c "1017 ,madd" rbfdts.0 
		echo "1 node - 1 thread" >> err.log
		grep -i "sum,error" rbfdts.* >> err.log
		grep -i "trace,error" rbfdts.* >> err.log
		cat err.log
	    done
	    
	    
	    # --------------------------------------------------------------------- Parallel Run 
	    SEQUENTIAL=0
	    for CHOL_CACHEOPT in 0  
	    do 
		NpList=($Npx)
		for Nt in 7 #$(seq 2 7)
		do
		    for Node in 1 # $(seq 2 9)
		    do
			Nbx=$[ $Npx / $Node / $BlkSz ]
    			NnList=($Node)
    			NtList=($Nt)
    			NbList=($Nbx)
    			run_loop $CholSpeedUp
    			if test "$(grep -i "timeout" rbfdts.*)" != ""
			then 
			    return
    			fi
    			echo "$Node nodes - $Nt threads" >> err.log
    			grep -i "sum,error" rbfdts.* >> err.log
    			grep -i "trace,error" rbfdts.* >> err.log
		    done #Node
		done #Nt
	    done #CacheOpt 
	done #Npx
    done #BlkSize
	    #python drawrecs.py $REC_FILE "$REC_FILE.filtered" $CholSpeedUp >> $STATISTICS_FILE

    cat err.log

}