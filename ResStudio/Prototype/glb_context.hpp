#ifndef __GLB_CONTEXT_HPP__
#define __GLB_CONTEXT_HPP__

#include <list>
#include <cstdio>
#include <sstream>
#include "data.hpp"
#include "config.hpp"

using namespace std;
extern int me;
class IContext;
class Config;
class ContextHostPolicy;

typedef struct {
  int fromVersion;
  int i,j;
  string fromCtx,toCtx;
}PropagateInfo;

typedef struct {
  int children,seq;
}LevelInfo;


#define BEGIN_CONTEXT_BY(counter,a,b,c) if(begin_context(this,a,b,c,counter)){
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
  list<DataRange *> getWriteRange(){
    return writeRange;
  }
  list<DataRange *> getReadRange(){return readRange;}
  void clear(){readRange.clear();writeRange.clear();}
};

/*==================================================================================*/
class GlobalContext
{
private:
  int s,TaskCount;
  int counters[9];

  list <GlobalContext *> children;
  list <PropagateInfo *> lstPropTasks;
  list <LevelInfo>       lstLevels;
  vector < list <PropagateInfo *> > nodesPropTasks;

  Config        *cfg;
  ContextHeader *ctxHeader;

  ContextHostPolicy   *hpContext;
  IHostPolicy  *hpData,*hpTask,*hpTaskRead,*hpTaskAdd,*hpTaskPropagate;

public :
  GlobalContext(){
    s=-1;
    ctxHeader = new ContextHeader;
    LevelInfo li;
    li.seq = 0;
    li.children = 0;
    lstLevels.push_back(li);
  }
  ~GlobalContext() {
    delete ctxHeader;
  }
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
  void           setConfiguration       (Config *_cfg);
  void           dumpStatistics         (Config *cfg);
  void           updateVersions         (IContext* ctx,ContextHeader *);
  void 	         endContext             ()		    {}
  void           beginContext           ()		    { s++;}
  void           setID                  (int id)	    { s=id;}
  void           incrementCounter       (Counters c,int v=1){counters[c] +=v;}
  void           setTaskCount           (int t)		    {TaskCount = t;}
  int            getID                  () 		    {return s;}
  int            getDepth               ()		    {return lstLevels.size();}
  int *          getCounters            () 		    {return counters; }
  IHostPolicy   *getDataHostPolicy      ()		    {return hpData;}
  IHostPolicy   *getTaskHostPolicy      ()		    {return hpTask;}
  IHostPolicy   *getTaskReadHostPolicy  ()		    {return hpTaskRead;}
  IHostPolicy   *getTaskAddHostPolicy   ()		    {return hpTaskAdd;}
  ContextHeader *getHeader              ()          {return ctxHeader;}
  IHostPolicy   *getTaskPropagatePolicy ()		    {return hpTaskPropagate;}
  ContextHostPolicy   *getContextHostPolicy   ()		    {return hpContext;}


  /*------------------------------*/
  void setPolicies (IHostPolicy *dhp,
		    IHostPolicy *thp,
		    ContextHostPolicy *chp,
		    IHostPolicy *trp,
		    IHostPolicy *tap,
		    IHostPolicy *tpp){
    hpContext       = chp;
    hpData          = dhp;
    hpTask          = thp;
    hpTaskRead      = trp;
    hpTaskAdd       = tap;
    hpTaskPropagate = tpp;
  }

  /*------------------------------*/
  void resetCounters() {
    counters[EnterContexts] = 0 ;
    counters[SkipContexts ] = 0 ;
    counters[TaskRead     ] = 0 ;
    counters[VersionTrack ] = 0 ;
    counters[TaskInsert   ] = 0 ;
    counters[TaskPropagate] = 0 ;
    counters[PropagateSize] = 0 ;
    counters[CompCost     ] = 0 ;
    counters[CommCost     ] = 0 ;
    s=-1;
    LevelInfo li;
    li.seq = 0;
    li.children = 0;
    lstLevels.clear();
    lstLevels.push_back(li);
    hpTaskAdd->reset();
  }

  /*------------------------------*/
  int  dumpPropagations(list < PropagateInfo *> prop_info){
    list < PropagateInfo *>::iterator it;
    typedef long data_handle; // ToDo: replace with real data handle
    int msg_size = 0 ;
    for (it = prop_info.begin(); it != prop_info.end();++it ) {
      PropagateInfo &P=*(*it);
      //printf ("  @A(%d,%d) [%s%d]== [%s0]\n",P.i,P.j,P.fromCtx.c_str(),P.fromVersion,P.toCtx.c_str());
      msg_size += sizeof (data_handle) +  P.fromCtx.size() + P.toCtx.size() + sizeof(P.fromVersion);
    }
  }

  /*------------------------------*/
  void createPropagateTasks(list<DataRange *>  dr){
    list<DataRange *> :: iterator it = dr.begin();
    for ( ; it != dr.end(); ++it ) {
      for ( int r=(*it)->row_from; r<= (*it)->row_to;r++){
	for ( int c=(*it)->col_from; c<= (*it)->col_to;c++){
	  IData &A=*((*it)->d);
	  PropagateInfo *p = new PropagateInfo();
	  p->fromVersion = A(r,c)->getCurrentVersion();
	  p->fromCtx.assign( getLevelString());
	  p->i = r; p->j = c;
	  lstPropTasks.push_back(p);
	}
      }
    }
  }

  /*------------------------------*/
  void createPropagateTasks(){
    if( !getTaskPropagatePolicy()->isAllowed( hpContext,me) )
      return ;
    createPropagateTasks( ctxHeader->getWriteRange() );
    createPropagateTasks( ctxHeader->getReadRange()  );
  }

  /*------------------------------*/
  void sendPropagateTasks(){
    list <PropagateInfo *>::iterator it;
    //if( !getTaskPropagatePolicy()->isAllowed(hpContext,me ) )      return ;
    for ( it = lstPropTasks.begin(); it != lstPropTasks.end();++it){
      int p = getDataHostPolicy()->getHost(Coordinate ((*it)->i,(*it)->j));
      (*it)->toCtx = getLevelString();
      nodesPropTasks[p].push_back((*it));
    }
    //send porpagation tasks
    for ( int p= 0; p < cfg->getProcessors(); p++){
      if (nodesPropTasks[p].size()==0) continue;
      if ( p != me ) {
	//printf ("  @send propagte:to %d \n",p) ;
	int msg_size =dumpPropagations(nodesPropTasks[p]);
	incrementCounter(TaskPropagate);
	incrementCounter(PropagateSize,msg_size);

      }
      nodesPropTasks[p].clear();
    }
    lstPropTasks.clear();
  }

  /*------------------------------*/
  void downLevel(){
    LevelInfo li  = lstLevels.back();
    li.seq = li.children;
    li.children = 0;
    lstLevels.push_back(li);
  }

  /*------------------------------*/
  void upLevel(){
    LevelInfo  li = lstLevels.back();
    lstLevels.pop_back();
    LevelInfo  liP = lstLevels.back();
    lstLevels.pop_back();
    liP.children ++;
    lstLevels.push_back(liP);
  }

  /*------------------------------*/
  int getLevelID(int level){
    list <LevelInfo>::iterator it=lstLevels.begin();
    for ( it  = lstLevels.begin();
	  it != lstLevels.end(),level>0; ++it,--level ) ;
    return (*it).seq ;
  }

  /*------------------------------*/
  string getLevelString(){
    list <LevelInfo>::iterator it;
    ostringstream _s;
    for ( it = lstLevels.begin(); it != lstLevels.end(); ++it ) {
      _s << (*it).seq <<".";
    }
    return _s.str();
  }

  /*------------------------------*/
  void dumpLevels(char c=' '){
    printf("%c:\n%s\n",c,getLevelString().c_str());
  }

  /*------------------------------*/
   bool isAnyOwnedBy(ContextHeader* hdr,int me){
     if ( isAnyOwnedBy(hdr->getWriteRange(),me))
       return true;
     return isAnyOwnedBy(hdr->getReadRange(),me);
   }

  /*------------------------------*/
  bool isAnyOwnedBy(list<DataRange*> dr,int me){
    list<DataRange*>::iterator  it = dr.begin();
    for ( ; it != dr.end() ;++it ){
      for ( int r = (*it)->row_from; r <=(*it)->row_to; r++){
	for ( int c = (*it)->col_from; c <=(*it)->col_to; c++){
	  IData &d=*((*it)->d);
	  bool b = d(r,c)->isOwnedBy(me) ;
	  if ( b )
	    return true;
	}
      }
    }
    return false;
  }


};

#endif //__GLB_CONTEXT_HPP__
