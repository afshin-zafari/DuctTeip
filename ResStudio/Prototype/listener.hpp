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
  DataAccess data_request;
  int state,host;
public :
   IListener(){}
  ~IListener(){}
};

#endif //__LISTENER_HPP__
