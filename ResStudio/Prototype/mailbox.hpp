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
  /*-------------------------------------------------------------------------------*/
  /*=============================================================*/
  
};

#endif //__MAILBOX_HPP__
