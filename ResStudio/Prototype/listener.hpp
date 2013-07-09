#ifndef __LISTENER_HPP__
#define __LISTENER_HPP__

#include <string>
#include <cstdio>
#include <vector>
#include <list>
#include "task.hpp"

class IListener
{
private:
  DataAccess *data_request;
  int state,host,handle;
  unsigned long comm_handle;
public :
  IListener(){}
  IListener(DataAccess* _d , int _host):data_request(_d) , host(_host){
    printf("listener ctor: data%s\n",data_request->data->getName().c_str());
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
