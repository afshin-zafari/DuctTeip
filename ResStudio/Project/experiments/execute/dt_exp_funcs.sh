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
