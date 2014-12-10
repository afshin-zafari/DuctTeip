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
  IListener(){
    message_buffer = NULL;
    data_request==NULL;
  }
  IListener(DataAccess* _d , int _host):data_request(_d) , host(_host){
    comm_handle = -1;
    received=false;
    int size = data_request->getPackSize()+ sizeof(host);
    message_buffer = new MessageBuffer(size,0);
  }
  ~IListener(){
    if  ( message_buffer != NULL ) 
      delete message_buffer;
  }

  int           getHandle     ()                 { return handle;      }
  void          setHandle     (int h )           { handle = h ;        } 
  unsigned long getCommHandle ()                 { return comm_handle; }
  void          setCommHandle (unsigned long ch) { comm_handle = ch ;  }
  void          setHost       (int h )           { host = h ;          }
  int           getHost       ()                 { return host;        }
  int           getCount()  { return count;}
  void          setCount(int c ) { count = c; } 
  IData *getData() { 
    return data_request->data;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion & getRequiredVersion(){
    return data_request->required_version;
  }
  /*--------------------------------------------------------------------------*/
  void setDataRequest ( DataAccess *dr){
    data_request = dr;
  }
  /*--------------------------------------------------------------------------*/
   void setRequiredVersion(DataVersion rhs){
    data_request->required_version = rhs;
  }
  /*--------------------------------------------------------------------------*/
  void dump(){
    if (!DUMP_FLAG)
      return;
    data_request->required_version.dump();
  }
  /*--------------------------------------------------------------------------*/
  void setSource(int s ) { source =s ; }
  int getSource(){return source;}
  void setDataSent(bool sent){ data_sent = sent ;}
  bool isDataRemote(){return (data_request->data->getHost() != me );}
  bool isDataSent(){ return data_sent;  }
  void setReceived(bool r) { received = r;}
  bool isReceived() { return received;}
  /*--------------------------------------------------------------------------*/
  bool isDataReady(){
    bool isReady = (data_request->data->getRunTimeVersion(IData::READ) == data_request->required_version);
    data_request->data->dump();
    data_request->data->getRunTimeVersion(IData::READ).dump();
    data_request->required_version.dump();
    return isReady;
  }
  /*--------------------------------------------------------------------------*/
  long getPackSize(){
    DataHandle dh;
    DataVersion dv;
    return sizeof(host) + dh.getPackSize() + dv.getPackSize();
  }
  /*--------------------------------------------------------------------------*/
  MessageBuffer *serialize(){
    int offset =0 ;
    serialize(message_buffer->address,offset,message_buffer->size);
    return message_buffer;
  }
  /*--------------------------------------------------------------------------*/
  void serialize(byte *buffer, int &offset, int max_length){
    copy<int>(buffer,offset,host);
    data_request->data->getDataHandle()->serialize(buffer,offset,max_length);
    data_request->required_version.serialize(buffer,offset,max_length);
  }
  /*--------------------------------------------------------------------------*/
  void deserialize(byte *buffer, int &offset, int max_length);
  void checkAndSendData(MailBox * mailbox);
  /*--------------------------------------------------------------------------*/
};
/*============================ IListener =====================================*/

#endif //__LISTENER_HPP__
