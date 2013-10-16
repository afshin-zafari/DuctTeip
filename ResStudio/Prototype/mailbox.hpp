#ifndef __MAILBOX_HPP__
#define __MAILBOX_HPP__

#include "basic.hpp"
#include "network.hpp"

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
  MailBoxEvent(byte *b,int l,int t,int s):
    buffer(b),length(l),tag(t),host(s)
  {}
  MailBoxEvent(){buffer = NULL; tag = host = length = 0 ;}
  ~MailBoxEvent(){
  }
  void dump(){
    if ( !DUMP_FLAG)
      return;
    printf("@EVENT:%s, ",direction == Received?"Received":"Sent");
    if (tag ==1 ) printf("Task");
    if (tag ==2 ) printf("Data");
    if (tag ==3 ) printf("Lsnr");
    if (tag ==5 ) printf("TerminateOK");
    printf("\n");
  }
};
class MailBox
{
private:
  INetwork *comm;
public:
  MailBox(INetwork *net):comm(net){}
  enum MessageTag{
    TaskTag=1,
    DataTag,
    ListenerTag,
    PropagationTag,
    TerminateOKTag,
    TerminateCancelTag
  } ;
  unsigned long  send(byte *buffer, int length, MessageTag tag, int destination){
    unsigned long comm_handle  = comm->send(buffer,length,(int)tag,destination);
    return comm_handle ;
  }

  bool getEvent(byte *data_buffer,MailBoxEvent *event,bool wait = false){
    int length,source,found=0,tag,
      mbox_tags[6]= {DataTag,TaskTag,ListenerTag,PropagationTag,TerminateOKTag,TerminateCancelTag};
    unsigned long handle;
    

    for ( int i =0 ; i< 6; i++) {
      found =  comm->probe(mbox_tags[i],&source,&length,wait);
      if ( !found ) continue;
      event->direction = MailBoxEvent::Received;
      TRACE_LOCATION;
      if (  mbox_tags[i] == DataTag ) 
	event->buffer = data_buffer;
      else	
	event->buffer = new byte[length];
      PRINT_IF(0)("mbox buffer mem:%p,sz:%d\n",event->buffer,length);
      event->length = length;
      event->host   = source;
      event->tag = mbox_tags[i];
      PRINT_IF(0)("1mbox malloc sz:%d\n",length);
      comm->receive(event->buffer,length,event->tag,source);
      PRINT_IF(0)("2mbox malloc sz:%d\n",length);
      TRACE_LOCATION;
      return true;
    }

    if (!found){
      found = comm->isAnySendCompleted(&tag,&handle);
      if ( (tag == TerminateOKTag) || (tag ==  TerminateCancelTag) ) 
	return false;
      if ( found ) {
	event->tag = tag;
	event->handle = handle;
	event->direction = MailBoxEvent::Sent;
	return true;
      }
      return false;
    }

    return false;
  }
  
  
};

#endif //__MAILBOX_HPP__
