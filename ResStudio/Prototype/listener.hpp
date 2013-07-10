#ifndef __LISTENER_HPP__
#define __LISTENER_HPP__

#include <string>
#include <cstdio>
#include <vector>
#include <list>
#include "task.hpp"

extern int me;
class IListener
{
private:
  DataAccess *data_request;
  int state,host,handle,source;
  unsigned long comm_handle;
  bool data_sent,received;
public :
  IListener(){}
  IListener(DataAccess* _d , int _host):data_request(_d) , host(_host){
    printf("listener ctor: data%s\n",data_request->data->getName().c_str());
    received=false;
  }
  ~IListener(){}

  int           getHandle     ()                 { return handle;      }
  void          setHandle     (int h )           { handle = h ;        } 
  unsigned long getCommHandle ()                 { return comm_handle; }
  void          setCommHandle (unsigned long ch) { comm_handle = ch ;  }
  void          setHost       (int h )           { host = h ;          }
  int           getHost       ()                 { return host;        }
  IData *getData() { 
    return data_request->data;
  }
  void setSource(int s ) { source =s ; }
  int getSource(){return source;}
  void setDataSent(bool sent){ data_sent = sent ;}
  bool isDataRemote(){return (data_request->data->getHost() != me );}
  bool isDataSent(){ return data_sent;  }
  void setReceived(bool r) { received = r;}
  bool isReceived() { return received;}
  bool isDataReady(){
    bool isReady = (data_request->data->getRequestVersion() == data_request->required_version);
    printf("**data :%s\n",data_request->data->getName().c_str());
    printf("**lsnr check, data rtver:%d , req ver :%d, required ver:%d\n",
	   data_request->data->getRunTimeVersion(IData::READ) ,
	   data_request->data->getRequestVersion() ,
	   data_request->required_version);
    return isReady;
  }
  void serialize(byte *buffer, int &offset, int max_length){
    copy<int>(buffer,offset,host);
    data_request->data->getDataHandle()->serialize(buffer,offset,max_length);
    int ver = data_request->required_version;
    copy<int>(buffer,offset,ver);
    bool ready=data_request->ready;
    copy<bool>(buffer,offset,ready);
  }
  void deserialize(byte *buffer, int &offset, int max_length);
};

#endif //__LISTENER_HPP__
