#ifndef __GLB_CONTEXT_HPP__
#define __GLB_CONTEXT_HPP__

#include <list>
#include <cstdio>
#include "data.hpp"
#include "config.hpp"

using namespace std;

class IContext;


class ContextHeader
{
private:
  list<DataRange *> readRange,writeRange;
public :
  void addDataRange(IData::AccessType daxs,DataRange *dr){
    if (daxs == IData::READ)
      readRange.push_back(dr);
    else
      writeRange.push_back(dr);
  }
  list<DataRange *> getWriteRange(){return writeRange;}
  list<DataRange *> getReadRange(){return readRange;}
};


class GlobalContext
{
private:
  int s;
  int counters[6];
  GlobalContext *parent;
  list<GlobalContext *> children;
  IHostPolicy *hpContext,*hpData,*hpTask,*hpTaskRead,*hpTaskAdd;
public :
  GlobalContext(){s=0;}
  enum Counters{
    Contexts,
    SummaryRead,
    SummaryWrite,
    TaskAdd,
    VersionTrack,
    TaskInsert
  };
  int getID() {return s;}
  void setID(int id) { s=id;}
  bool Boundry(IContext* ctx,bool c);
  bool BoundryWithHeader(IContext* ctx,ContextHeader *);
  int *getCounters() {return counters; }
  void increment(Counters c ){counters[c] ++;}
  IHostPolicy *getContextHostPolicy(){return hpContext;}
  IHostPolicy *getDataHostPolicy(){return hpData;}
  IHostPolicy *getTaskHostPolicy(){return hpTask;}
  IHostPolicy *getTaskReadHostPolicy(){return hpTaskRead;}
  IHostPolicy *getTaskAddHostPolicy(){return hpTaskAdd;}
  void setPolicies(IHostPolicy *dhp,
		   IHostPolicy *thp,
		   IHostPolicy *chp,
		   IHostPolicy *trp,
		   IHostPolicy *tap){
		   hpContext = chp;
		   hpData = dhp;
		   hpTask = thp;
		   hpTaskRead=trp;
		   hpTaskAdd=tap;
		   }
  void resetCounters() {
    printf("reset Counters\n");
    counters[Contexts] = 0;
    counters[SummaryRead]  = 0;
    counters[SummaryWrite] = 0;
    counters[TaskAdd]  = 0;
    counters[VersionTrack] = 0 ;
    printf("reset Counters,finished\n");
  }
  void dumpStatistics(Config *cfg);
};

#endif //__GLB_CONTEXT_HPP__
