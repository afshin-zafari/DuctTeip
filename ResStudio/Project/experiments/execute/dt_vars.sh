#-----------------------   Global Use ---------------------------------------------
Yes=1;YES=1;yes=1
No=0;NO=0;no=0
# ----------------------   Environment --------------------------------------------
SOLARIS=1;UPPMAX=2;LINUX=3
TINTIN=4;KALKYL=5;
env=$SOLARIS;
# ----------------------   Parameters Range ---------------------------------------



NpMax=6500 ; NnMax=7 ; NtMax=8
NpMin=6000 ; NnMin=0 ; NtMin=4
SNp=6400   ; SNn=    ; SNt=8
SMtd=3     ; SNb=2
# ----------------------   Execution Parameters -----------------------------------

PURE_MPI=0
Nb2=1
DrawRecs=1
SEQUENTIAL=0
NpUnit=1
PART_CNT=1


# ----------------------   Events ------------------------------------------------
TSND=1013;TACK=200;DSND=1011;DACK=2;TRCV=100;DRCV=1019;TRUN=300;TFIN=500;SRCH=1022
E1=1020  ; E2=1021 ; T1=2000000;T2=2200000;T3=2250000
TASK_1BY1=1;TASK_ONCE=0;
BCAST_PIPE=1;BCAST_12ALL=2;

# ----------------------   Load Balancing Parameters-------------------------------
LBM_NONE=0; LBM_TASK_STEALING=1;LBM_TASK_EXPORTING=2;LBM_NODES_GROUPING=3;
LBM_METHOD=$LBM_NONE
GROUP_SIZE=1
NODE_CAPACITY=10
CHOLESKY_TEST=0
CHOL_LOAD_BAL=1
CHOL_COMM_PIPE=0
PRINT_TASK_LIST=0


grep_flags=""
if test "$env" != "$SOLARIS"
then 
    grep_flags="--binary-file=text"
fi

Rt=10
SEQ_CHUNK=1000
proj_code=p2009014
job_name=dts01



SeqParComp=1;    NpPerCore=2;     
NbComp=5;        SpeedUp=6;       PureMPIComp=7; Cholesky=8;
FixedSize=9;     NodeImpact=10;   WeakScaling=11; ScaleUp=12;
CholSpeedUp=13; PartPerNode=14
