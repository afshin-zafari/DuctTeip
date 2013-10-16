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
  int state,host,handle,source;
  unsigned long comm_handle;
  bool data_sent,received;
  MessageBuffer *message_buffer;
public :
  IListener(){
  }
  IListener(DataAccess* _d , int _host):data_request(_d) , host(_host){
    comm_handle = -1;
    received=false;
    int size = data_request->getPackSize()+ sizeof(host);
    message_buffer = new MessageBuffer(size,0);
  }
  ~IListener(){
	  TRACE_LOCATION;
    delete message_buffer;
	  TRACE_LOCATION;
  }

  int           getHandle     ()                 { return handle;      }
  void          setHandle     (int h )           { handle = h ;        } 
  unsigned long getCommHandle ()                 { return comm_handle; }
  void          setCommHandle (unsigned long ch) { comm_handle = ch ;  }
  void          setHost       (int h )           { host = h ;          }
  int           getHost       ()                 { return host;        }
  IData *getData() { 
    return data_request->data;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion getRequiredVersion(){
    return data_request->required_version;
  }
  /*--------------------------------------------------------------------------*/
  void dump(){
    if (!DUMP_FLAG)
      return;
    export(V_DUMP,O_LSNR);
    export_info(",ListenerData:%s,",getData()->getName().c_str());
    export_int(source);
    export_int(host);
    export_long(comm_handle);
    export_int(handle);
    export_int(data_sent);
    data_request->required_version.dump();
    export_end(V_DUMP);
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
    export(V_CHECK_RDY,O_LSNR);
    data_request->data->dump();
    export_info("Runtime-Write-Version","");
    data_request->data->getRunTimeVersion(IData::READ).dump();
    export_info("Required-Version","");
    data_request->required_version.dump();
    export_int(isReady);
    export_end(V_CHECK_RDY);    
    return isReady;
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
