#ifndef __HOSTPOLICY_HPP__
#define __HOSTPOLICY_HPP__

#include <string>
#include <iostream>
#include <vector>
#include <list>

#include "data_basic.hpp"

using namespace std;


class ProcessGrid;
class IData;
class IContext;
class ContextHeader;
class ContextHostPolicy;
class Coordinate;

class IHostPolicy
{
protected :
    ProcessGrid *PG;
public:
    IHostPolicy(ProcessGrid *pg):PG(pg) {}

    int getHost(Coordinate c ,int ndim = 2);
    virtual bool isInRow(int me , int r ) ;
    virtual bool isInCol(int me , int c ) ;
    virtual bool isAllowed(IContext *,ContextHeader *)
    {
        return true;
    }
    virtual bool isAllowed(ContextHostPolicy *,int)
    {
        return true;
    }
    virtual void reset() {}

};
/*==================Context Host Policy ==========================*/
class ContextHostPolicy : public IHostPolicy
{
public:
    typedef enum
    {
        PROC_GROUP_CYCLIC=1,
        ALL_ENTER=2
    } chpType;
private:
    chpType active_policy;
    vector <int> groupCount;
    int groupCounter;

public :
    ContextHostPolicy (ProcessGrid *pg) : IHostPolicy(pg) {}
    ContextHostPolicy (chpType c , ProcessGrid &pg) : IHostPolicy(&pg),active_policy(c)
    {
        groupCounter = -1;
    }
    int getHost ( IContext * context)
    {
        return -1;
    }
    bool isAllowed(ContextHostPolicy *,int )
    {
        return false;
    }
    bool isAllowed(IContext *,ContextHeader*);
    bool canAllEnter()
    {
        return  (active_policy == ALL_ENTER);
    }
    void setGroupCount(int l1,int l2, int l3=1)
    {
        groupCount.push_back(l1);
        groupCount.push_back(l2);
        groupCount.push_back(l3);
    }
    void setGroupCounter(int c)
    {
        groupCounter = c;
    }
    int getGroupCounter()
    {
        return groupCounter;
    }
    void getHostRange(int *lower, int *upper);
};

/*==================Data Host Policy ==========================*/

class DataHostPolicy : public IHostPolicy
{
public:
    typedef enum
    {
        BLOCK_CYCLIC=1,
        ROW_CYCLIC,
        COLUMN_CYCLIC
    } dhpType;
private:
    dhpType active_policy;
public :
    DataHostPolicy (ProcessGrid *pg) : IHostPolicy(pg) {}
    DataHostPolicy (dhpType t,ProcessGrid &pg)  : IHostPolicy(&pg) {}
    int getHost(int i, int j )
    {
        return -1;
    }
};


/*==================Task Host Policy ==========================*/
class TaskHostPolicy : public IHostPolicy
{
public:
    typedef enum
    {
        WRITE_DATA_OWNER=1
    } thpType;
private:
    thpType active_policy;
public :
    TaskHostPolicy (ProcessGrid *pg) : IHostPolicy(pg) {}
    TaskHostPolicy (thpType t,ProcessGrid &pg) : IHostPolicy(&pg) {}
    int getHost ( list <IData*> in_data,list <IData*> out_data)
    {
        return -1;
    }
};
/*==================Task Read Policy ==========================*/
class TaskReadPolicy : public IHostPolicy
{
public:
    typedef enum
    {
        ALL_GROUP_MEMBERS=1,
        ALL_READ_ALL=2,
        ONE_READ_ALL=3
    } trpType;
private:
    trpType active_policy;
public :
    TaskReadPolicy (ProcessGrid *pg) : IHostPolicy(pg) {}
    TaskReadPolicy (trpType trp,ProcessGrid &pg) : IHostPolicy(&pg),active_policy(trp) {}
    int getHost ( list <IData*> in_data,list <IData*> out_data)
    {
        return -1;
    }
    bool isAllowed(IContext *c,ContextHeader *hdr);
    bool isAllowed(ContextHostPolicy *,int )
    {
        return false;
    }
};

/*==================Task Add Policy ==========================*/
class TaskAddPolicy : public IHostPolicy
{
public:
    typedef enum
    {
        WRITE_DATA_OWNER=1,
        NOT_OWNER_CYCLIC,
        ROOT_ONLY
    } tapType;
private:
    tapType active_policy;
    int not_owner_count;
public :
    TaskAddPolicy (ProcessGrid *pg) : IHostPolicy(pg),not_owner_count(0) {}
    TaskAddPolicy (tapType tap,ProcessGrid &pg) : IHostPolicy(&pg),active_policy(tap),not_owner_count(0) {}
    int getHost ( list <IData*> in_data,list <IData*> out_data)
    {
        return -1;
    }
    bool isAllowed(IContext *c,ContextHeader *hdr);
    bool isAllowed(ContextHostPolicy *,int )
    {
        return false;
    }
    void reset()
    {
        not_owner_count=0;
    }
    void setPolicy(tapType t)
    {
        active_policy = t;
    }
};




#endif //__HOSTPOLICY_HPP__
