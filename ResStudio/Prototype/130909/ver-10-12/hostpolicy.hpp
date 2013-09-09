#ifndef __HOSTPOLICY_HPP__
#define __HOSTPOLICY_HPP__

#include <string>
#include <iostream>
#include <vector>
#include <list>
#include "data.hpp"



class ProcessGrid;
class IData;
class IContext;
class ContextHeader;

class IHostPolicy
{
protected :
  ProcessGrid *PG;
public:
  IHostPolicy(ProcessGrid *pg):PG(pg){}
  virtual int getHost(Coordinate c ,int ndim = 2) {
    if ( ndim == 1 )
      return (c.bx % PG->getProcessorCount() ) ;
    return PG->getProcessor(c.by,c.bx);
  }
  virtual bool isInRow(int me , int r ) {
    return PG->isInRow(me,r);
  }
  virtual bool isInCol(int me , int c ) {
    return PG->isInCol(me,c);
  }
  virtual bool isAllowed(IContext *,ContextHeader *){}

};
/*==================Context Host Policy ==========================*/
class ContextHostPolicy : public IHostPolicy
{
public:
  typedef enum  {
    PROC_GROUP_CYCLIC=1
  }chpType;
private:
  chpType active_policy;
public :
  ContextHostPolicy (ProcessGrid *pg) : IHostPolicy(pg){}
  ContextHostPolicy (chpType c , ProcessGrid &pg) : IHostPolicy(&pg),active_policy(c){}
  int getHost ( IContext * context){}
  bool isAllowed(IContext *,ContextHeader*);
};

/*==================Data Host Policy ==========================*/

class DataHostPolicy : public IHostPolicy
{
public:
  typedef enum  {
    BLOCK_CYCLIC=1,
    ROW_CYCLIC,
    COLUMN_CYCLIC
  }dhpType;
private:
  dhpType active_policy;
public :
  DataHostPolicy (ProcessGrid *pg) : IHostPolicy(pg){}
  DataHostPolicy (dhpType t,ProcessGrid &pg)  : IHostPolicy(&pg){}
};


/*==================Task Host Policy ==========================*/
class TaskHostPolicy : public IHostPolicy
{
public:
  typedef enum  {
    WRITE_DATA_OWNER=1
  }thpType;
private:
  thpType active_policy;
public :
  TaskHostPolicy (ProcessGrid *pg) : IHostPolicy(pg){}
  TaskHostPolicy (thpType t,ProcessGrid &pg) : IHostPolicy(&pg){}
  int getHost ( list <IData*> in_data,list <IData*> out_data){}
};
/*==================Task Read Policy ==========================*/
class TaskReadPolicy : public IHostPolicy
{
public:
  typedef enum  {
    ALL_GROUP_MEMBERS=1
  }trpType;
private:
  trpType active_policy;
public :
  TaskReadPolicy (ProcessGrid *pg) : IHostPolicy(pg){}
  TaskReadPolicy (trpType trp,ProcessGrid &pg) : IHostPolicy(&pg){}
  int getHost ( list <IData*> in_data,list <IData*> out_data){}
};

/*==================Task Add Policy ==========================*/
class TaskAddPolicy : public IHostPolicy
{
public:
  typedef enum  {
    GROUP_LEADER=1
  }trpType;
private:
  trpType active_policy;
public :
  TaskAddPolicy (ProcessGrid *pg) : IHostPolicy(pg){}
  TaskAddPolicy (trpType trp,ProcessGrid &pg) : IHostPolicy(&pg){}
  int getHost ( list <IData*> in_data,list <IData*> out_data){}
};



#endif //__HOSTPOLICY_HPP__
