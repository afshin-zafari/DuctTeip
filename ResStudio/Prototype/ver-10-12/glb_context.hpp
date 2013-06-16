#ifndef __GLB_CONTEXT_HPP__
#define __GLB_CONTEXT_HPP__

#include <list>
#include <cstdio>
#include "data.hpp"
#include "config.hpp"

using namespace std;

class IContext;

typedef struct {
  int fromVersion,fromCtx;
  int i,j;
  string ctxName;
}PropagateInfo;

typedef struct {
  int children,seq;
}LevelInfo;


#define BEGIN_CONTEXT(a,b,c) if(begin_context(this,a,b,c)){
#define END_CONTEXT()        }end_context(this);


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
  list<GlobalContext *> children;
  ContextHeader *ctxHeader;
  IHostPolicy *hpContext,*hpData,*hpTask,*hpTaskRead,*hpTaskAdd;
  list <PropagateInfo *> lstPropTasks;
  vector < list <PropagateInfo *> > nodesPropTasks;
  list <LevelInfo> lstLevels;

public :
  GlobalContext(){
    s=0;
    ctxHeader   =new ContextHeader;
    LevelInfo li;
    li.seq = 0;
    li.children = 0;
    lstLevels.push_back(li);

  }
  ~GlobalContext() {
    delete ctxHeader;
  }
  ContextHeader *getHeader() {return ctxHeader;}
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
  void updateVersions(IContext* ctx,ContextHeader *);
  int *getCounters() {return counters; }
  void incrementCounter(Counters c ){counters[c] ++;}
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
  void createPropagateTasks(IData *M){
    IData &A=*M;
    int Nb = 1;
    for ( int i = 0;i<Nb;i++)
      for ( int j = 0;i<Nb;i++){
	PropagateInfo *p = new PropagateInfo();
	p->fromVersion = A(i,j)->getCurrentVersion();
	p->fromCtx = this->getID();
	p->i = i; p->j = j;
	p->ctxName = "";//getFullName();
	lstPropTasks.push_back(p);
      }
    movePropTasksByNode();
  }
  void movePropTasksByNode(){
    list <PropagateInfo *>::iterator it;
    for ( it = lstPropTasks.begin(); it != lstPropTasks.end();++it){
      int p = 0 ; //glbCtx.getDataHostPolicy()->getHost((*it)->i,(*it)->j)
      nodesPropTasks[p].push_back((*it));
    }
    lstPropTasks.clear();
  }
  void downLevel(){
    LevelInfo li  = lstLevels.back();
    li.seq = li.children;
    li.children = 0;
    lstLevels.push_back(li);
    dumpLevels('d');

  }
  void upLevel(){
    LevelInfo  li = lstLevels.back();
    lstLevels.pop_back();
    LevelInfo  liP = lstLevels.back();
    lstLevels.pop_back();
    liP.children ++;
    lstLevels.push_back(liP);
    dumpLevels('u');
  }
  void dumpLevels(char c){
    list <LevelInfo>::iterator it;
    printf("%c:\n",c);
    for ( it = lstLevels.begin(); it != lstLevels.end(); ++it ) {
      printf("%d.",(*it).seq);
    }
    printf("\n");
  }

};

#endif //__GLB_CONTEXT_HPP__
