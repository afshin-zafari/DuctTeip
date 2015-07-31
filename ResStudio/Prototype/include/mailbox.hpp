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
  int direction,length,tag,host;
  byte *buffer;
  ulong  handle;  
  MemoryItem *memory;
  /*-------------------------------------------------------------------------------*/
              MailBoxEvent(byte *b,int l,int t,int s);
  void        setMemoryItem(MemoryItem *m);
  MemoryItem *getMemoryItem();
  MailBoxEvent &operator =(MailBoxEvent &rhs);

   MailBoxEvent();
  ~MailBoxEvent();
  void dump();
};
typedef list <MailBoxEvent *> MailBoxEventList;
/*===============================================================================*/
class MailBox
{
private:
  INetwork        *comm;
  MailBoxEvent     last_receive_event,shared_event;
  MailBoxEventList post_list;
  bool self_terminate_received, self_terminate_sent;;
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
    SelfTerminate,
    FindIdleTag,//8
    FindBusyTag,
    MigrateTaskTag,
    MigrateDataTag,
    DeclineMigrateTag,
    AcceptMigrateTag,
    MigratedTaskOutDataTag
  } ;
  /*-------------------------------------------------------------------------------*/
  ulong  	 send                  (byte *, int, int, int,bool wait=false);
  inline void 	 getLRNeighbors        (int ,int *,int *);
  bool 		 getEvent              (MemoryManager *,MailBoxEvent *,bool *,bool wait = false);
  void 		 waitForAnySendComplete(MailBoxEvent *);
  void 		 waitForAnyReceive     (MemoryManager *,MailBoxEvent *);
  bool 		 getSelfTerminate      (int send_or_recv);
};

#endif //__MAILBOX_HPP__
