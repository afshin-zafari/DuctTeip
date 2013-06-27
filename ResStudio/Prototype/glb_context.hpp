#ifndef __GLB_CONTEXT_HPP__
#define __GLB_CONTEXT_HPP__

#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "data.hpp"
#include "config.hpp"
#include "engine.hpp"

#define TRACE_LOCATION printf("%s , %d\n",__FILE__,__LINE__);

typedef unsigned char byte;
using namespace std;

extern int me;
class IContext;
class Config;
class ContextHostPolicy;

struct PropagateInfo{
public:
  int fromVersion;
  int i,j;
  string fromCtx,toCtx;
  DataHandle data_handle;
  template<class T> 
  void copy(byte * b,int &o,T a){
    memcpy(b+o,(char *)&a,sizeof(a));
    o +=  sizeof(a);
  }
  template<class T> 
  void paste(byte * b,int &o,T a){
    memcpy((char *)&a,b+o,sizeof(a));
    o +=  sizeof(a);
  }
  void deserializeContext(byte *buffer,int &offset,int max_length,string &ctx){
    int level_count, ctx_level_id ;
    paste<int>(buffer,offset,level_count);
    ostringstream s;
    TRACE_LOCATION;
    for(int i=0; i < level_count; i++){
      TRACE_LOCATION;
      paste<int>(buffer,offset,ctx_level_id);
      TRACE_LOCATION;
      s << ctx_level_id << ".";
      TRACE_LOCATION;
    }
      TRACE_LOCATION;
    ctx = s.str();      
      TRACE_LOCATION;
  }
  void   serializeContext(byte *buffer,int &offset,int max_size,string ctx){
    istringstream s(ctx);
    int ctx_level_id,level_count=0,local_offset = offset;
    char c;
    local_offset += sizeof(int);
    TRACE_LOCATION;
    while ( !s.eof() ) {
      TRACE_LOCATION;
      s >> ctx_level_id >> c;
      TRACE_LOCATION;
      copy<int>(buffer,local_offset,ctx_level_id);
      TRACE_LOCATION;
      level_count ++;
    }
    TRACE_LOCATION;
     copy<int>(buffer,offset,level_count);
    TRACE_LOCATION;
     offset = local_offset;
    TRACE_LOCATION;
    printf ("offset : %d , max_size:%d\n",offset,max_size);
  }
  int serialize(byte *buffer,int max_length){
    int offset = 0 ;
    TRACE_LOCATION;
    copy<int>(buffer,offset,fromVersion);
    TRACE_LOCATION;
    copy<int>(buffer,offset,i);
    TRACE_LOCATION;
    copy<int>(buffer,offset,j);
    TRACE_LOCATION;
    copy<unsigned long>(buffer,offset,data_handle.context_handle);
    TRACE_LOCATION;
    copy<unsigned long>(buffer,offset,data_handle.   data_handle);
    TRACE_LOCATION;
    /*
    serializeContext(buffer,offset,max_length,fromCtx);
    TRACE_LOCATION;
    serializeContext(buffer,offset,max_length,  toCtx);
    TRACE_LOCATION;
    */
    return offset;
  }
  void deserialize(byte * buffer , int &offset,int max_length){
      TRACE_LOCATION;
    paste<int>(buffer,offset,fromVersion);
      TRACE_LOCATION;
    paste<int>(buffer,offset,i);
      TRACE_LOCATION;
    paste<int>(buffer,offset,j);
      TRACE_LOCATION;
    paste<unsigned long>(buffer,offset,data_handle.context_handle);
      TRACE_LOCATION;
    paste<unsigned long>(buffer,offset,data_handle.   data_handle);
      TRACE_LOCATION;
      /*
    deserializeContext(buffer,offset,max_length,fromCtx);
      TRACE_LOCATION;
    deserializeContext(buffer,offset,max_length,  toCtx);
      TRACE_LOCATION;
      */
    
  }
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
  void addDataRange(IData::AccessType daxs,DataRange *dr){
    if (daxs == IData::READ)
      readRange.push_back(dr);
    else
      writeRange.push_back(dr);
  }
  list<DataRange *> getWriteRange(){
    return writeRange;
  }
  list<DataRange *> getReadRange (){return readRange;}
  void clear(){readRange.clear();writeRange.clear();}
};

/*==================================================================================*/
class GlobalContext
{
private:
  int  sequence,TaskCount;
  int  counters[9];
  bool with_propagation;

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
   GlobalContext(){
    sequence=-1;
    ctxHeader = new ContextHeader;
    LevelInfo li;
    li.seq = 0;
    li.children = 0;
    lstLevels.push_back(li);
    last_context_handle = 0 ;
    last_data_handle = 0 ;
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
  ContextHeader *getHeader              ()          {return ctxHeader;}
  IHostPolicy   *getTaskPropagatePolicy ()		    {return hpTaskPropagate;}

  ContextHostPolicy   *getContextHostPolicy   ()		    {return hpContext;}
  ContextHandle       *createContextHandle(){
    ContextHandle *ch = new ContextHandle ; 
    *ch = last_context_handle++;
    return ch;
  }
  DataHandle          *createDataHandle (){
    DataHandle *dh = new DataHandle ; 
    dh->data_handle = last_data_handle++ ;
    return dh;
  }

  IContext *getContextByHandle ( ContextHandle ch) ;
  IData    *getDataByHandle(DataHandle *d);
  void      doPropagation(bool f) {with_propagation = f;}
  void      addContext(IContext *c){
    children.push_back(c);
  }

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
    sequence=-1;
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
      printf ("  @A(%d,%d) [%s%d]== [%s0]\n",P.i,P.j,P.fromCtx.c_str(),P.fromVersion,P.toCtx.c_str());
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
	  p->data_handle = *A(r,c)->getDataHandle();
	  lstPropTasks.push_back(p);
	}
      }
    }
  }
  /*------------------------------*/
  void createPropagateTasks(){
    if ( with_propagation ) {
      if( !getTaskPropagatePolicy()->isAllowed( hpContext,me) )
	return ;
      createPropagateTasks( ctxHeader->getWriteRange() );
      createPropagateTasks( ctxHeader->getReadRange()  );
    } 
  }
  
  /*------------------------------*/
  void testPropagatePacking(){
    list < PropagateInfo *> prop_info;
    DataHandle dh (10,25);

    PropagateInfo **p= new PropagateInfo*[4];
    for ( int i=0; i<4;i++)
      p[i] = new PropagateInfo;
    p[0]->fromCtx = string("1.2.1.");
    p[0]->toCtx=string("1.2.2.");
    p[0]->i = p[0]->j = 22;
    p[0]->fromVersion = 7;
    p[0]->data_handle = dh;
    prop_info.push_back(p[0]);

    p[1]->fromCtx = string("1.2.1.");
    p[1]->toCtx=string("1.2.2.");
    p[1]->i = p[1]->j = 22;
    p[1]->fromVersion = 7;
    p[1]->data_handle = dh;  
    //    prop_info.push_back(p[1]);

    p[2]->fromCtx = string("1.2.1.");
    p[2]->toCtx=string("1.2.2.");
    p[2]->i = p[2]->j = 22;
    p[2]->fromVersion = 7;
    p[2]->data_handle = dh;
    //prop_info.push_back(p[2]);

    p[3]->fromCtx = string("1.2.1.");
    p[3]->toCtx=string("1.2.2.");
    p[3]->i = p[3]->j = 22;
    p[3]->fromVersion = 7;
    p[3]->data_handle = dh;
    //prop_info.push_back(p[3]);

    byte *buffer;
    int length;
    TRACE_LOCATION;
    packPropagateTask(prop_info,10); // pack and send
    TRACE_LOCATION;
    dtEngine.receivePropagateTask(buffer,&length);
    TRACE_LOCATION;
    unpackPropagateTask(buffer,length);
    TRACE_LOCATION;

  }
  /*------------------------------*/
  void unpackPropagateTask(byte *buffer,int length){
    PropagateInfo P;
    int offset = 0 ;
    TRACE_LOCATION;
    while (offset <length) {
      TRACE_LOCATION;
      P.deserialize(buffer,offset,length);
      TRACE_LOCATION;
      printf ("  @A(%d,%d) [%s%d]== [%s0]\n",P.i,P.j,P.fromCtx.c_str(),P.fromVersion,P.toCtx.c_str());
      TRACE_LOCATION;
      dtEngine.addPropagateTask(&P);
      TRACE_LOCATION;
    }
    TRACE_LOCATION;
  }
  /*------------------------------*/
  void packPropagateTask(list < PropagateInfo *> prop_info,int dest_host){
    list < PropagateInfo *>::iterator it;
    int msg_max_size = 1024; //prop_info.size() * sizeof(PropagateInfo) ;
    int msg_len = 0;
    //    unsigned char *msg_buffer= new unsigned char [msg_max_size];
    byte  *msg_buffer= (byte *)malloc ( msg_max_size);
    TRACE_LOCATION;
    for (it = prop_info.begin(); it != prop_info.end();++it ) {
      PropagateInfo &P=*(*it);
      TRACE_LOCATION;
      msg_len += P.serialize(msg_buffer + msg_len,msg_max_size);      
    }
    TRACE_LOCATION;
    dtEngine.sendPropagateTask(msg_buffer,msg_len,dest_host);
    TRACE_LOCATION;
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
	packPropagateTask(nodesPropTasks[p],p);
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
  int  getLevelID(int level){
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
  void testHandles();


};

#endif //__GLB_CONTEXT_HPP__
