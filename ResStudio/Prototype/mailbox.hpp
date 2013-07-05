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
    return comm->send(buffer,length,(int)tag,destination);
  }
  bool getEvent(MailBoxEvent *event,bool wait = false){
    int length,source;
    int found = comm->probe((int)TaskTag,&source,&length,wait);
    if (!found) {
      int tag;
      unsigned long handle;
      found = comm->isAnySentCompleted(&tag,&handle);
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
    event->tag    = (int)TaskTag;
    printf("mb.msg recv tag:%d, task-tag:%d , event:%p\n",event->tag,(int)TaskTag,event);
    TRACE_LOCATION;
    comm->receive(buffer,length,(int)TaskTag,source);
    printf("mb.msg recv tag:%d, task-tag:%d\n",event->tag,(int)TaskTag);
    TRACE_LOCATION;
    return true;
  }
  
  
};

#endif //__MAILBOX_HPP__
