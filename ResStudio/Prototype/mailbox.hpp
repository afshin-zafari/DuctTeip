#ifndef __MAILBOX_HPP__
#define __MAILBOX_HPP__

#include "basic.hpp"
#include "network.hpp"


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
  int  send(byte *buffer, int length, MessageTag tag, int destination){
    return comm->send(buffer,length,(int)tag,destination);
  }
  int checkInbox();
  
};

#endif //__MAILBOX_HPP__
