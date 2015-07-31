#ifndef __GLB_CONTEXT_HPP__ 
#define __GLB_CONTEXT_HPP__

#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "config.hpp"
#include "engine.hpp"
#include "hostpolicy.hpp"
#include "data.hpp"

using namespace std;
extern engine dtEngine;

extern int me;
class IContext;
class Config;
//class ContextHostPolicy;

struct PropagateInfo{
public:
  int        fromVersion,i,j;
  string     fromCtx,toCtx;
  DataHandle data_handle;

  void dump();
  /*-----------------------------------------------------------------------------------------*/
  void deserializeContext(byte *buffer,int &offset,int max_length,string &ctx);
  void   serializeContext(byte *buffer,int &offset,int max_size,string ctx);
  int    serialize(byte *buffer,int &offset,int max_length);
  void deserialize(byte *buffer,int &offset,int max_length);
};

typedef struct {
  int children,seq;
}LevelInfo;

typedef unsigned long int ContextHandle;


#define DONT_CARE_COUNTER -1
#define BEGIN_CONTEXT_EX(counter,func,r1,r2,w) {bool _b_ = begin_context(this,r1,r2,w,counter,0,1) ; if ( !_b_) {func;} else{
#define END_CONTEXT_EX()                             }end_context(this);}
#define BEGIN_CONTEXT_ALL(counter,from,to,r1,r2,w) if(begin_context(this,r1,r2,w,counter,from,to)){
#define BEGIN_CONTEXT_BY(counter,a,b,c) if(begin_context(this,a,b,c,counter,0,1)){
#define BEGIN_CONTEXT(a,b,c) if(begin_context(this,a,b,c,DONT_CARE_COUNTER,0,1)){
#define END_CONTEXT()        }end_context(this);


class ContextHeader
{
private:
  list<DataRange *> readRange,writeRange;
public :
  /*-----------------------------------------------------------------------------------------*/
  void addDataRange(IData::AccessType daxs,DataRange *dr);
    list<DataRange *> getWriteRange();
  list<DataRange *> getReadRange (){return readRange;}
  void clear(){readRange.clear();writeRange.clear();}
};

/*==================================================================================*/
typedef struct {
  IContext *context;
  unsigned long task_key;
} TaskKernel;

class GlobalContext
{
private:
  int  sequence,TaskCount;
  int  counters[9];

  list <PropagateInfo *> lstPropTasks;
  list <IContext *>      children;
  list <LevelInfo>       lstLevels;

  vector < list <PropagateInfo *> > nodesPropTasks;

  Config              *cfg;
  ContextHeader       *ctxHeader;
  ContextHostPolicy   *hpContext;
  IHostPolicy         *hpData,*hpTask,*hpTaskRead,*hpTaskAdd,*hpTaskPropagate;
  unsigned long        last_context_handle,last_data_handle;
  
  
public :
  /*-----------------------------------------------------------------------------------------*/
  GlobalContext();
  ~GlobalContext() ;
  enum Counters{
    EnterContexts,
    SkipContexts ,
    TaskRead     ,
    VersionTrack ,
    TaskInsert   ,
    TaskPropagate,
    PropagateSize,
    CompCost,
    CommCost
  };
  enum DataType{
    InputData,
    OutputData,
    InOutData
  };
  /*-----------------------------------------------------------------------------------------*/
  void           setConfiguration       (Config *_cfg);
  void           dumpStatistics         (Config *cfg);
  void           updateVersions         (IContext* ctx,ContextHeader *);
  void 	         endContext             ()		    {}
  void           beginContext           ()		    { sequence++;}
  void           setID                  (int id)	    { sequence=id;}
  void           incrementCounter       (Counters c,int v=1){counters[c] +=v;}
  void           setTaskCount           (int t)		    {TaskCount = t;}
  int            getID                  () 		    {return sequence;}
  int            getDepth               ()		    {return lstLevels.size();}
  int           *getCounters            () 		    {return counters; }
  IHostPolicy   *getDataHostPolicy      ()		    {return hpData;}
  IHostPolicy   *getTaskHostPolicy      ()		    {return hpTask;}
  IHostPolicy   *getTaskReadHostPolicy  ()		    {return hpTaskRead;}
  IHostPolicy   *getTaskAddHostPolicy   ()		    {return hpTaskAdd;}
  ContextHeader *getHeader              ()                  {return ctxHeader;}
  ContextHostPolicy   *getContextHostPolicy   ()		    {return hpContext;}
  bool canAllEnter();

  /*-----------------------------------------------------------------------------------------*/
  ContextHandle       *createContextHandle();
  DataHandle          *createDataHandle ();
  IContext *getContextByHandle ( ContextHandle ch) ;
  IData    *getDataByHandle(DataHandle *d);
  //  void      doPropagation(bool f) {with_propagation = f;}
  void      addContext(IContext *c);
  void getLocalNumBlocks(int *mb, int *nb);
  int getNumThreads(){return cfg->getNumThreads();}
  void setPolicies (IHostPolicy *dhp,
		    IHostPolicy *thp,
		    ContextHostPolicy *chp,
		    IHostPolicy *trp,
		    IHostPolicy *tap){
    hpContext       = chp;
    hpData          = dhp;
    hpTask          = thp;
    hpTaskRead      = trp;
    hpTaskAdd       = tap;
  }

  void resetCounters() ;
  DataRange *getIndependentData(IContext *ctx,int data_type);
  void resetVersiosOfDataRange(DataRange *  data_range);
  void resetVersiosOfDataRangeList(list<DataRange *>  dr);
  void resetVersiosOfHeaderData();
  void downLevel();
  void upLevel();
  int  getLevelID(int level);
  string getLevelString();
  void dumpLevels(char c=' ');
  bool isAnyOwnedBy(ContextHeader* hdr,int me);
  bool isAnyOwnedBy(list<DataRange*> dr,int me);
};
extern GlobalContext glbCtx;



#endif //__GLB_CONTEXT_HPP__
