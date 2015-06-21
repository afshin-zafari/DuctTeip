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
  void setMemoryItem(MemoryItem *m);
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
  unsigned long  send(byte *buffer, int length, int tag, int destination,bool wait=false);
  inline void getLRNeighbors(int org,int *left,int *right);
  unsigned long prepareTerminateReceiveForHost(int source);
  void  prepareTerminateReceive(int source1,int source2, int source3);
  void  prepareListenerReceive(int length,int host=-1);
  unsigned long prepareDataReceive(MemoryItem  *mi,int source,unsigned long key);
  void dumpPosts();
  bool getPostEvent(MailBoxEvent *event,int tag,unsigned long  handle);
  bool checkPostedReceives(MailBoxEvent *event);
  bool getEvent(MemoryManager *memman,MailBoxEvent *event,bool *completed,bool wait = false);
};

#endif //__MAILBOX_HPP__
