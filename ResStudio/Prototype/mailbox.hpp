#ifndef __MAILBOX_HPP__
#define __MAILBOX_HPP__

#include "basic.hpp"
#include "network.hpp"
#include "dt_log.hpp"
#include "memory_manager.hpp"
/*===============================================================================*/
struct MailBoxEvent{
public :
  enum EventDirection {Received,Sent};
  int direction;
  // for received events
  byte *buffer;
  int length;
  // for sent events
  unsigned long handle;  
  // for both events
  int tag,host;
  MemoryItem *memory;
  /*-------------------------------------------------------------------------------*/
  MailBoxEvent(byte *b,int l,int t,int s):
    buffer(b),length(l),tag(t),host(s)
  {
    memory=NULL;
  }
  /*-------------------------------------------------------------------------------*/
  void setMemoryItem(MemoryItem *m){memory = m ;}
  /*-------------------------------------------------------------------------------*/
  MemoryItem *getMemoryItem(){return memory;}
  /*-------------------------------------------------------------------------------*/
  MailBoxEvent(){buffer = NULL; tag = host = length = 0 ;}
  /*-------------------------------------------------------------------------------*/
  ~MailBoxEvent(){ }
  /*-------------------------------------------------------------------------------*/
  MailBoxEvent &operator =(MailBoxEvent &rhs){
    direction = rhs.direction;
    buffer    = rhs.buffer;
    length    = rhs.length;
    tag       = rhs.tag;
    host      = rhs.host;
    handle    = rhs.handle;
    memory    = rhs.memory;
    return *this;
  }
  /*-------------------------------------------------------------------------------*/
  void dump(){
    if (!DUMP_FLAG)
      return;
    printf("@EVENT:%s, ",direction == Received?"Received":"Sent");
    if (tag ==1 ) printf("Task");
    if (tag ==2 ) printf("Data");
    if (tag ==3 ) printf("Lsnr");
    if (tag ==5 ) printf("TerminateOK");
    printf("\n");
  }
};
/*===============================================================================*/
class MailBox
{
private:
  INetwork *comm;
  MailBoxEvent last_receive_event,shared_event;
  list <MailBoxEvent *> post_list;
public:
  /*-------------------------------------------------------------------------------*/
  MailBox(INetwork *net):comm(net){}
  /*-------------------------------------------------------------------------------*/
  enum MessageTag{
    TaskTag=1,
    DataTag,
    ListenerTag,
    PropagationTag,
    TerminateOKTag,
    TerminateCancelTag,
    FindIdleTag,//7
    FindBusyTag,
    MigrateTaskTag,
    MigrateDataTag,
    DeclineMigrateTag,
    AcceptMigrateTag,
    MigratedTaskOutDataTag
  } ;
  /*-------------------------------------------------------------------------------*/
  unsigned long  send(byte *buffer, int length, int tag, int destination,bool wait=false){
    unsigned long comm_handle  = comm->send(buffer,length,(int)tag,destination,wait);
    if ( tag > 1000 ) {
      printf("@@@ tag:%d, len:%d, to:%d\n",tag,length,destination);
    }
    return comm_handle ;
  }
  /*-------------------------------------------------------------------------------*/
  inline void getLRNeighbors(int org,int *left,int *right){
    int n = comm->get_host_count();
    *right=(org+1) %n;
    if ( (org%2 ==1)){
      *left=org-1;
    }
    else{
      *left = org -1;
      if ( *left < 0 ) 
	*left +=n;
    }
  }

  /*-------------------------------------------------------------------------------*/
  unsigned long prepareTerminateReceiveForHost(int source){
    unsigned long comm_handle;
    MailBoxEvent  *event = new MailBoxEvent();
    int length;
    
    length = sizeof(int);
    event->memory =  NULL;
    event->buffer = new byte[length];
    event->direction = MailBoxEvent::Received;
    event->length = length;
    event->host   = source;
    event->tag = TerminateOKTag ; 
    event->handle  = comm->postTerminateReceive(event->buffer,event->length,TerminateOKTag,source);
    PRINT_IF(POSTPRINT)("posted TermOK for host:%d,handle:%ld\n",source,event->handle);
    post_list.push_back(event);
    dumpPosts();
    return event->handle;
  }

  /*-------------------------------------------------------------------------------*/
  void  prepareTerminateReceive(int source1,int source2, int source3){
#if TERMINATE_TREE == 0     
    getLRNeighbors(me,&source1,&source2);
#endif
    if (source1 >=0)
      prepareTerminateReceiveForHost(source1);
    if (source2 >=0)
      prepareTerminateReceiveForHost(source2);
    if (source3 >=0)
      prepareTerminateReceiveForHost(source3);
    return ;
  }

  /*-------------------------------------------------------------------------------*/
  void  prepareListenerReceive(int length,int host=-1){
    unsigned long comm_handle;
    PRINT_IF(POSTPRINT)("lsnr pack size:%d\n",length);
    int node_count = comm->get_host_count();
    for (int source =0 ; source < node_count; source++){
      if ( source ==  me ) continue;
      if ( host != -1 ) 
	if ( source != host ) 
	  continue;
      MailBoxEvent  *event = new MailBoxEvent();
      event->memory =  NULL;
      event->buffer = new byte[length];
      event->direction = MailBoxEvent::Received;
      event->length = length;
      event->host   = source;
      event->tag = ListenerTag ; 
      event->handle  = comm->postListenerReceive(event->buffer,event->length,ListenerTag,source);
      PRINT_IF(POSTPRINT)("posted Lsnr for host:%d,handle:%ld\n",source,event->handle);
      post_list.push_back(event);
      dumpPosts();
    }
    
    return ;
  }
  /*-------------------------------------------------------------------------------*/
  unsigned long prepareDataReceive(MemoryItem  *mi,int source,unsigned long key){
    unsigned long comm_handle;
    MailBoxEvent  *event = new MailBoxEvent();
    event->memory = mi;
    event->buffer = mi->getAddress();
    event->direction = MailBoxEvent::Received;
    event->length = mi->getSize();
    event->host   = source;
    event->tag = DataTag ; 
    event->handle  = comm->postDataReceive(event->buffer,event->length,DataTag,source,key);
    PRINT_IF(POSTPRINT)("posted Data for host:%d,handle:%ld\n",source,event->handle);
    post_list.push_back(event);
    dumpPosts();
    return event->handle;
  }
  /*-------------------------------------------------------------------------------*/
  void dumpPosts(){
    if (!(POSTPRINT)) return;
    list<MailBoxEvent*>::iterator it;
    printf("-----------post events list----------\n");
    for (it = post_list.begin(); it != post_list.end(); it ++){
      MailBoxEvent*ev=(*it);
      printf ("h:%ld, t:%d\n",ev->handle  ,ev->tag);
    }
  }
  /*-------------------------------------------------------------------------------*/
  bool getPostEvent(MailBoxEvent *event,int tag,unsigned long  handle){
    list<MailBoxEvent*>::iterator it;
    for (it = post_list.begin(); it != post_list.end(); it ++){
      MailBoxEvent*ev=(*it);
      PRINT_IF(POSTPRINT) ("h:%ld, inh:%ld , t:%d,int:%d\n",ev->handle , handle ,ev->tag , tag);
      if (ev->handle != handle || ev->tag != tag)
	continue;
      *event = *ev;
      post_list.erase(it);
      return true;
    }
    printf("error: not found event\n");
    return false;
  }
  /*-------------------------------------------------------------------------------*/
  bool checkPostedReceives(MailBoxEvent *event){
    int length,source,tag;
    unsigned long handle;
    bool found;
    found = comm->isAnyPostCompleted(&tag,&handle);
    if ( !found ) 
      return false;
    found = getPostEvent(event,tag,handle);
    return found;
  }

  /*-------------------------------------------------------------------------------*/
  bool getEvent(MemoryManager *memman,MailBoxEvent *event,bool *completed,bool wait = false){
    int tag,source,length;
    *completed = false;
    bool received=comm->probe(&tag,&source,&length,wait);
    if ( !received){
      addLogEventStart("AnySendCompleted",DuctteipLog::AnySendCompleted);
      unsigned long handle;
      bool found = comm->isAnySendCompleted(&tag,&handle);
      addLogEventEnd("AnySendCompleted",DuctteipLog::AnySendCompleted);
      if ( (tag != TerminateOKTag) && (tag !=  TerminateCancelTag) ) 
	if ( found ) {
	  if(0)printf("send complete: %d,%ld\n",tag,handle);
	  event->tag = tag;
	  event->handle = handle;
	  event->direction = MailBoxEvent::Sent;
	  *completed =  true;
	  return true;
	}
      return false;
    }
    if(0)printf("received : tag=%d,from %d,len=%d \n",tag,source,length);
    event->direction = MailBoxEvent::Received;
    event->length = length;
    event->host   = source;
    event->tag = tag ; 
    event->handle  = 0;
    if ( tag == MigrateDataTag ||
	 tag == DataTag ||
	 tag == MigratedTaskOutDataTag){
      event->memory = memman->getNewMemory();
      event->buffer = event->memory->getAddress();
    }
    else{      
      event->memory =  NULL;
      event->buffer = new byte[length];
    }
    int res=comm->receive(event->buffer,length,tag,source,true);
    *completed = (res==0);
    return true;
    
  }
  /*-------------------------------------------------------------------------------*/
  bool getEventOld(MemoryManager *memman,MailBoxEvent *event,bool *completed,bool wait = false){

    int length,source,tag;
    unsigned long handle;
    bool found=false;
    if ( checkPostedReceives(event) ) {
      PRINT_IF(0)("post completed for tag:%d\n",event->tag);
      if ( event->tag == 10 || event->tag ==2){
	double * M= (double*)(event->buffer+192);
	int size = (event->length -192)/sizeof(double);
	double sum=0.0;
	for (int i=0;i<size;i++)
	  sum+= M[i];
	if(0)printf("@Checksum z :%lf\n",sum);
	/*
	  for ( int i=0;i<5;i++)
	  for ( int j =0;j<5;j++){
	  sum += M[];
	  }
	*/
      }
      *completed = true;
      return true;
    }
    int dlbtags[7]={    
      FindIdleTag,
      FindBusyTag,
      MigrateTaskTag,
      MigrateDataTag,
      DeclineMigrateTag,
      AcceptMigrateTag,
      MigratedTaskOutDataTag
    };
    tag=-1;
    found = comm->probeTags(dlbtags,7,&source,&length,&tag);
    event->tag = -1;
    if(found){
      event->tag = tag;
      event->length = length;
      event->host = source;
      event->direction = MailBoxEvent::Received;
      //printf("DLB rcv src:%d,tag:%d,len:%d\n",source,tag,length);
      if (length <=2){
	byte p;
	int res=comm->receive(&p,1,tag,source,true);
	*completed = true;
	return true;
      }
      if (  tag == MigrateDataTag || tag == MigratedTaskOutDataTag) {
        event->memory = memman->getNewMemory();
	event->buffer = event->memory->getAddress();
        event->memory->setState(MemoryItem::InUse);
	if(0)printf("DLB DATA rcv src:%d,tag:%d,len:%d,buf:%p\n",source,tag,length,event->buffer);
      }
      else{
	  event->buffer = new byte[event->length];
      }
      
	int res=comm->receive(event->buffer,length,tag,source,true);
	if (  tag == MigrateDataTag || tag == MigratedTaskOutDataTag) {
	  if (0){
	    double sum = 0.0,*contents=(double *)(event->buffer+192);
	    long size = (length-192)/sizeof(double);
	    for ( long i=0; i< size; i++)
	      sum += contents[i];
	    printf("+++sum i , ---------,%lf adr:%p\n",sum,contents);
	  }
	}
      
      if(0)printf("et , el, es : %d,%d,%d\n",event->tag,event->length,event->host);
      *completed = true;
      return true;
    }
    if (!found){
      addLogEventStart("AnySendCompleted",DuctteipLog::AnySendCompleted);
      found = comm->isAnySendCompleted(&tag,&handle);
      addLogEventEnd("AnySendCompleted",DuctteipLog::AnySendCompleted);
      if ( (tag != TerminateOKTag) && (tag !=  TerminateCancelTag) ) 
	if ( found ) {
	  event->tag = tag;
	  event->handle = handle;
	  event->direction = MailBoxEvent::Sent;
	  *completed =  true;
	  return true;
	}
    }
    return false;
  }
  /*-------------------------------------------------------------------------------*/
  /*=============================================================*/
  
};

#endif //__MAILBOX_HPP__
