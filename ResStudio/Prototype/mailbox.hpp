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
  ~MailBoxEvent(){free(buffer);}
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
    PropagationTag
  };
  unsigned long  send(byte *buffer, int length, MessageTag tag, int destination){
    TRACE_LOCATION;
    unsigned long comm_handle  = comm->send(buffer,length,(int)tag,destination);
    TRACE_LOCATION;
    return comm_handle ;
  }

  bool getEvent(MailBoxEvent *event,bool wait = false){
    int length,source,data_received=0,listener_received=0,task_received=0;
    data_received = comm->probe((int)DataTag,&source,&length,wait);
    if (!data_received) {
      task_received = comm->probe((int)TaskTag,&source,&length,wait);
      if ( !task_received) {
	listener_received = comm->probe((int)ListenerTag,&source,&length,wait);
      }
    }
    if (!data_received && !task_received && !listener_received) {
      int tag;
      unsigned long handle;
      int found = comm->isAnySendCompleted(&tag,&handle);
      if ( found ) {
	event->tag = tag;
	event->handle = handle;
	event->direction = MailBoxEvent::Sent;
	return true;
      }
      return false;
    }
    TRACE_LOCATION;
    event->direction = MailBoxEvent::Received;
    byte *buffer = (byte *)malloc(length);
    TRACE_LOCATION;
    event->buffer = buffer;
    event->length = length;
    event->host   = source;
    if ( data_received ) {
      event->tag    = (int)DataTag;
      printf("mb.msg recv tag:%d, data-tag:%d , event:%p\n",event->tag,(int)DataTag,event);
    }
    if ( task_received ) {
      event->tag    = (int)TaskTag;
      printf("mb.msg recv tag:%d, task-tag:%d , event:%p\n",event->tag,(int)TaskTag,event);
    }
    if ( listener_received ) {
      event->tag    = (int)ListenerTag;
      printf("mb.msg recv tag:%d, listener-tag:%d , event:%p\n",event->tag,(int)ListenerTag,event);
    }
    TRACE_LOCATION;
    comm->receive(buffer,length,event->tag,source);
    TRACE_LOCATION;
    return true;
  }
  
  
};

#endif //__MAILBOX_HPP__
