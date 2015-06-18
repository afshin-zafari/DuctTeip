#ifndef __LISTENER_HPP__ 
#define __LISTENER_HPP__

#include <string>
#include <cstdio>
#include <vector>
#include <list>
#include "task.hpp"

extern int me;
class MailBox;
/*============================ IListener =====================================*/
class IListener
{
private:
  DataAccess *data_request;
  int state,host,handle,source,count;
  unsigned long comm_handle;
  bool data_sent,received;
  MessageBuffer *message_buffer;
public :
  IListener();
  IListener(DataAccess* _d , int _host);
  ~IListener();

  int           getHandle     ()                 ;
  void          setHandle     (int h )           ;
  unsigned long getCommHandle ()                 ;
  void          setCommHandle (unsigned long ch) ;
  void          setHost       (int h )           ;
  int           getHost       ()                 ;
  int           getCount()  ;
  void          setCount(int c ) ;
  IData *getData() ;
  DataVersion & getRequiredVersion();
  void setDataRequest ( DataAccess *dr);
  void setRequiredVersion(DataVersion rhs);
  void dump();
  void setSource(int s ) ;
  int getSource();
  void setDataSent(bool sent);
  bool isDataRemote();
  bool isDataSent();
  void setReceived(bool r);
  bool isReceived() ;
  bool isDataReady();
  long getPackSize();
  MessageBuffer *serialize();
  void serialize(byte *buffer, int &offset, int max_length);
  void deserialize(byte *buffer, int &offset, int max_length);
  void checkAndSendData(MailBox * mailbox);
};
/*============================ IListener =====================================*/

#endif //__LISTENER_HPP__
