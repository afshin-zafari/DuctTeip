grep -n "VIZIT*(EVENT_TASK_CLEAND" *.F90
echo "------------VIZIT*(EVENT_TASK_CLEAND"
grep -n "VIZIT*(EVENT_TASK_CHECKED" *.F90
echo "------------VIZIT*(EVENT_TASK_CHECKED"
grep -n "VIZIT*(EVENT_TASK_STS_CHANGED" *.F90
echo "------------VIZIT*(EVENT_TASK_STS_CHANGED"
grep -n "VIZIT3(EVENT_TASK_SEND_REQUESTED" *.F90
echo "------------VIZIT3(EVENT_TASK_SEND_REQUESTED"
grep -n "VIZIT*(EVENT_TASK_RECEIVED" *.F90
echo "------------VIZIT*(EVENT_TASK_RECEIVED"
grep -n "VIZIT*(EVENT_TASK_ACK" *.F90
echo "------------VIZIT*(EVENT_TASK_ACK"
grep -n "VIZIT3(EVENT_TASK_STARTED" *.F90 	
echo "------------VIZIT3(EVENT_TASK_STARTED"
grep -n "VIZIT3(EVENT_TASK_RUNNING" *.F90
echo "------------VIZIT3(EVENT_TASK_RUNNING"
grep -n "VIZIT3(EVENT_TASK_FINISHED" *.F90 
echo "------------VIZIT3(EVENT_TASK_FINISHED"

grep -n "VIZIT*(EVENT_DATA_ADDED" *.F90 
echo "------------VIZIT*(EVENT_DATA_ADDED"
grep -n "VIZIT4(EVENT_DATA_DEP" *.F90 
echo "------------VIZIT4(EVENT_DATA_DEP"
grep -n "VIZIT*(EVENT_DATA_STS_CHANGED" *.F90 
echo "------------VIZIT*(EVENT_DATA_STS_CHANGED"
grep -n "VIZIT*(EVENT_DATA_POPULATED" *.F90 
echo "------------VIZIT*(EVENT_DATA_POPULATED"
grep -n "VIZIT3(EVENT_DATA_SEND_REQUESTED" *.F90 
echo "------------VIZIT3(EVENT_DATA_SEND_REQUESTED"
grep -n "VIZIT*(EVENT_DATA_RECEIVED" *.F90 
echo "------------VIZIT*(EVENT_DATA_RECEIVED"
grep -n "VIZIT*(EVENT_DATA_ACK" *.F90 
echo "------------VIZIT*(EVENT_DATA_ACK"

grep -n "VIZIT*(EVENT_LSNR_ADDED" *.F90          
echo "------------VIZIT*(EVENT_LSNR_ADDED"
grep -n "VIZIT*(EVENT_LSNR_DATA_RECEIVED" *.F90  
echo "------------VIZIT*(EVENT_LSNR_DATA_RECEIVED"
grep -n "VIZIT*(EVENT_LSNR_DATA_SENT" *.F90      
echo "------------VIZIT*(EVENT_LSNR_DATA_SENT"
grep -n "VIZIT*(EVENT_LSNR_STS_CHANGED" *.F90    
echo "------------VIZIT*(EVENT_LSNR_STS_CHANGED"
grep -n "VIZIT*(EVENT_LSNR_CLEANED" *.F90        
echo "------------VIZIT*(EVENT_LSNR_CLEANED"
grep -n "VIZIT3(EVENT_LSNR_SENT_REQUESTED" *.F90 
echo "------------VIZIT3(EVENT_LSNR_SENT_REQUESTED"
grep -n "VIZIT*(EVENT_LSNR_RECEIVED" *.F90 	   
echo "------------VIZIT*(EVENT_LSNR_RECEIVED"
grep -n "VIZIT*(EVENT_LSNR_ACK" *.F90 	       
echo "------------VIZIT*(EVENT_LSNR_ACK"
grep -n "VIZIT*(EVENT_ALL_TASK_FINISHED" *.F90
echo "------------VIZIT*(EVENT_ALL_TASK_FINISHED"