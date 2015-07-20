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
              MailBoxEvent(byte *b,int l,int t,int s);
  void        setMemoryItem(MemoryItem *m);
  MemoryItem *getMemoryItem();
   MailBoxEvent();
  ~MailBoxEvent();
  MailBoxEvent &operator =(MailBoxEvent &rhs);
  void dump();
};
/*===============================================================================*/
class MailBox
{
private:
  INetwork *comm;
  MailBoxEvent last_receive_event,shared_event;
  list <MailBoxEvent *> post_list;
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
  unsigned long  send                  (byte *, int, int, int,bool wait=false);
  inline void 	 getLRNeighbors        (int ,int *,int *);
  bool 		 getEvent              (MemoryManager *,MailBoxEvent *,bool *,bool wait = false);
  void 		 waitForAnySendComplete(MailBoxEvent *);
  void 		 waitForAnyReceive     (MemoryManager *,MailBoxEvent *);
  bool 		 getSelfTerminate      (int send_or_recv);
};

#endif //__MAILBOX_HPP__
