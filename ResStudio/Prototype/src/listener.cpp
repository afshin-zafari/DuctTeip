#include "listener.hpp"

IListener::IListener(){
  message_buffer = NULL;
  data_request==NULL;
}
IListener::IListener(DataAccess* _d , int _host):data_request(_d) , host(_host){
  comm_handle = -1;
  received=false;
  int size = data_request->getPackSize()+ sizeof(host);
  message_buffer = new MessageBuffer(size,0);
}
IListener::~IListener(){
  if  ( message_buffer != NULL ) 
    delete message_buffer;
}

int           IListener::getHandle     ()                 { return handle;      }
void          IListener::setHandle     (int h )           { handle = h ;        } 
unsigned long IListener::getCommHandle ()                 { return comm_handle; }
void          IListener::setCommHandle (unsigned long ch) { comm_handle = ch ;  }
void          IListener::setHost       (int h )           { host = h ;          }
int           IListener::getHost       ()                 { return host;        }
int           IListener::getCount()  { return count;}
void          IListener::setCount(int c ) { count = c; } 
IData *IListener::getData() { 
  return data_request->data;
}
/*--------------------------------------------------------------------------*/
DataVersion & IListener::getRequiredVersion(){
  return data_request->required_version;
}
/*--------------------------------------------------------------------------*/
void IListener::setDataRequest ( DataAccess *dr){
  data_request = dr;
}
/*--------------------------------------------------------------------------*/
void IListener::setRequiredVersion(DataVersion rhs){
  data_request->required_version = rhs;
}
/*--------------------------------------------------------------------------*/
void IListener::dump(){
  if (!DUMP_FLAG)
    return;
  data_request->required_version.dump();
}
/*--------------------------------------------------------------------------*/
void IListener::setSource(int s ) { source =s ; }
int IListener::getSource(){return source;}
void IListener::setDataSent(bool sent){ data_sent = sent ;}
bool IListener::isDataRemote(){return (data_request->data->getHost() != me );}
bool IListener::isDataSent(){ return data_sent;  }
void IListener::setReceived(bool r) { received = r;}
bool IListener::isReceived() { return received;}
/*--------------------------------------------------------------------------*/
bool IListener::isDataReady(){
  bool isReady = (data_request->data->getRunTimeVersion(IData::READ) == data_request->required_version);
  data_request->data->dump();
  data_request->data->getRunTimeVersion(IData::READ).dump();
  data_request->required_version.dump();
  return isReady;
}
/*--------------------------------------------------------------------------*/
long IListener::getPackSize(){
  DataHandle dh;
  DataVersion dv;
  return sizeof(host) + dh.getPackSize() + dv.getPackSize();
}
/*--------------------------------------------------------------------------*/
MessageBuffer *IListener::serialize(){
  int offset =0 ;
  serialize(message_buffer->address,offset,message_buffer->size);
  return message_buffer;
}
/*--------------------------------------------------------------------------*/
void IListener::serialize(byte *buffer, int &offset, int max_length){
  copy<int>(buffer,offset,host);
  data_request->data->getDataHandle()->serialize(buffer,offset,max_length);
  data_request->required_version.serialize(buffer,offset,max_length);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

