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

using namespace std;
extern engine dtEngine;

extern int me;
class IContext;
class Config;
class ContextHostPolicy;

struct PropagateInfo{
public:
  int        fromVersion,i,j;
  string     fromCtx,toCtx;
  DataHandle data_handle;

  void dump();
  /*-----------------------------------------------------------------------------------------*/
  void deserializeContext(byte *buffer,int &offset,int max_length,string &ctx){
    int level_count, ctx_level_id ;
    paste<int>(buffer,offset,&level_count);
    ostringstream s;
    for(int i=0; i < level_count; i++){
      paste<int>(buffer,offset,&ctx_level_id);
      s << ctx_level_id << ".";
    }
    ctx = s.str();      
  }
  /*-----------------------------------------------------------------------------------------*/
  void   serializeContext(byte *buffer,int &offset,int max_size,string ctx){
    int ctx_level_id,level_count=0,local_offset = offset;
    istringstream s(ctx);
    char c;//1.2.1.
    local_offset += sizeof(int);
    while ( !s.eof() ) {
      s >> ctx_level_id >> c;
      if ( s.eof() ) break;
      copy<int>(buffer,local_offset,ctx_level_id);
      level_count ++;
    }
     copy<int>(buffer,offset,level_count);
     offset = local_offset;
  }
  /*-----------------------------------------------------------------------------------------*/
  int    serialize(byte *buffer,int &offset,int max_length){
    copy<int>(buffer,offset,fromVersion);
    copy<int>(buffer,offset,i);
    copy<int>(buffer,offset,j);
    copy<unsigned long>(buffer,offset,data_handle.context_handle);
    copy<unsigned long>(buffer,offset,data_handle.   data_handle);
    serializeContext(buffer,offset,max_length,fromCtx);
    serializeContext(buffer,offset,max_length,  toCtx);
    return offset;
  }
  /*-----------------------------------------------------------------------------------------*/
  void deserialize(byte *buffer,int &offset,int max_length){
    paste<int>(buffer,offset,&fromVersion);
    paste<int>(buffer,offset,&i);
    paste<int>(buffer,offset,&j);
    paste<unsigned long>(buffer,offset,&data_handle.context_handle);
    paste<unsigned long>(buffer,offset,&data_handle.   data_handle);
    deserializeContext(buffer,offset,max_length,fromCtx);
    deserializeContext(buffer,offset,max_length,  toCtx);
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
  /*-----------------------------------------------------------------------------------------*/
  void addDataRange(IData::AccessType daxs,DataRange *dr){
    if (daxs == IData::READ)
      readRange.push_back(dr);
    else
      writeRange.push_back(dr);
  }
  /*-----------------------------------------------------------------------------------------*/
  list<DataRange *> getWriteRange(){
    return writeRange;
  }
  /*-----------------------------------------------------------------------------------------*/
  list<DataRange *> getReadRange (){return readRange;}
  /*-----------------------------------------------------------------------------------------*/
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
  /*-----------------------------------------------------------------------------------------*/
  GlobalContext(){
    sequence=-1;
    ctxHeader = new ContextHeader;
    LevelInfo li;
    li.seq = 0;
    li.children = 0;
    lstLevels.push_back(li);
    last_context_handle = 34 ;
    last_data_handle = 1000 ;
  }
  /*-----------------------------------------------------------------------------------------*/
  ~GlobalContext() {
    delete ctxHeader;
  }
  /*-----------------------------------------------------------------------------------------*/
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
  IHostPolicy   *getTaskPropagatePolicy ()		    {return hpTaskPropagate;}
  int            getPropagateCount      ()                  {return lstPropTasks.size();}
  ContextHostPolicy   *getContextHostPolicy   ()		    {return hpContext;}
  bool           canAllEnter(){return hpContext->canAllEnter();}
  /*-----------------------------------------------------------------------------------------*/
  ContextHandle       *createContextHandle(){
    ContextHandle *ch = new ContextHandle ; 
    *ch = last_context_handle++;
    return ch;
  }
  /*-----------------------------------------------------------------------------------------*/
  DataHandle          *createDataHandle (){
    DataHandle *dh = new DataHandle ; 
    dh->data_handle = last_data_handle++ ;
    return dh;
  }

  /*-----------------------------------------------------------------------------------------*/
  IContext *getContextByHandle ( ContextHandle ch) ;
  IData    *getDataByHandle(DataHandle *d);
  void      doPropagation(bool f) {with_propagation = f;}
  /*-----------------------------------------------------------------------------------------*/
  void      addContext(IContext *c){
    children.push_back(c);
  }
  /*-----------------------------------------------------------------------------------------*/
  void getLocalNumBlocks(int *mb, int *nb){
    *nb = cfg->getXLocalBlocks();
    *mb = cfg->getYLocalBlocks();
  }
  /*-----------------------------------------------------------------------------------------*/
  int getNumThreads(){return cfg->getNumThreads();}
  /*------------------------------*/
  /*
  void setEndContext(){
    if (last_context_boundry == EndContext)
      return ;
    //version chain: fromCtx = current ctx , fromVer = current ver
    last_context_boundry = EndContext;
    
  }

  void setBeginContext(){
    if (last_context_boundry == BeginContext){
      // version chain: toCtx = currentCtx
      return ;
    }
    //broadcast version chain
    last_context_boundry = EndContext;
  }
  */
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
  DataRange *getIndependentData(IContext *ctx,int data_type);
  /*------------------------------*/
  void initPropagateTasks(){
    list<IContext *>::iterator it;
    DataRange *ind_data;
    for(it = children.begin(); it != children.end(); it ++){
      ind_data =getIndependentData( (*it),InputData);
      if ( ind_data)
	createPropagateTasksFromRange(ind_data);

      ind_data =getIndependentData( (*it),OutputData);
      if ( ind_data)
	createPropagateTasksFromRange(ind_data);

      ind_data =getIndependentData( (*it),InOutData);
      if ( ind_data)
	createPropagateTasksFromRange(ind_data);
    }    
  }
  /*------------------------------*/
  int  dumpPropagations(list < PropagateInfo *> prop_info){
    list < PropagateInfo *>::iterator it;
    DataHandle  data_handle;
    int msg_size = 0 ;
    for (it = prop_info.begin(); it != prop_info.end();++it ) {
      PropagateInfo &P=*(*it);
      IData *d = getDataByHandle(&P.data_handle);
      
      printf ("  @%s(%d,%d) [%s%d]== [%s0]\n",d->getName().c_str(),P.i,P.j,P.fromCtx.c_str(),P.fromVersion,P.toCtx.c_str());
      msg_size += sizeof (data_handle) +  P.fromCtx.size() + P.toCtx.size() + sizeof(P.fromVersion);
    }
    return msg_size;
  }
  /*------------------------------*/
  void createPropagateTasks(list<DataRange *>  dr){
    list<DataRange *> :: iterator it = dr.begin();
    for ( ; it != dr.end(); ++it ) {
      createPropagateTasksFromRange((*it));
    }
  }
  /*------------------------------*/
  void createPropagateTasksFromRange(DataRange *  data_range){
    for ( int r=data_range->row_from; r<= data_range->row_to;r++){
      for ( int c=data_range->col_from; c<= data_range->col_to;c++){
	IData &A=*(data_range->d);
	PropagateInfo *p = new PropagateInfo();
	p->fromVersion = A(r,c)->getReadVersion().getVersion();
	p->fromCtx.assign( getLevelString());
	p->i = r; p->j = c;
	p->data_handle = *A(r,c)->getDataHandle();
	lstPropTasks.push_back(p);
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
  void resetVersiosOfDataRange(DataRange *  data_range){
    for ( int r=data_range->row_from; r<= data_range->row_to;r++){
      for ( int c=data_range->col_from; c<= data_range->col_to;c++){
	IData &A=*(data_range->d);
	A.resetVersion();
      }
    }    
  }
  /*------------------------------*/
  void resetVersiosOfDataRangeList(list<DataRange *>  dr){
    list<DataRange *> :: iterator it = dr.begin();
    for ( ; it != dr.end(); ++it ) {
      resetVersiosOfDataRange((*it));
    }
  }
  /*------------------------------*/
  void resetVersiosOfHeaderData(){
    resetVersiosOfDataRangeList(ctxHeader->getWriteRange() );
    resetVersiosOfDataRangeList(ctxHeader->getReadRange()  );
  }
  /*------------------------------*/
  void testPropagatePacking(){
    list < PropagateInfo *> prop_info;
    DataHandle dh (10,25);

    PropagateInfo **p= new PropagateInfo*[4];
    for ( int i=0; i<4;i++)
      p[i] = new PropagateInfo;
    p[0]->fromCtx = string("1.2.1.3.5.");
    p[0]->toCtx=string("1.2.2.");
    p[0]->i = p[0]->j = 22;
    p[0]->fromVersion = 7;
    p[0]->data_handle = dh;
    prop_info.push_back(p[0]);

    p[1]->fromCtx = string("1.2.1.3.");
    p[1]->toCtx=string("1.2.2.5.");
    p[1]->i = p[1]->j = 32;
    p[1]->fromVersion = 17;
    p[1]->data_handle = dh;  
    prop_info.push_back(p[1]);

    p[2]->fromCtx = string("1.2.1.");
    p[2]->toCtx=string("1.2.2.");
    p[2]->i = p[2]->j = 2987;
    p[2]->fromVersion = 21;
    p[2]->data_handle = dh;
    prop_info.push_back(p[2]);

    p[3]->fromCtx = string("1.2.1.");
    p[3]->toCtx=string("1.4.2.");
    p[3]->i = p[3]->j = 232;
    p[3]->fromVersion = 77;
    p[3]->data_handle = dh;
    prop_info.push_back(p[3]);

    byte *buffer=NULL;
    int length=0;
    packPropagateTask(prop_info,10); // pack and send
    //dtEngine.receivePropagateTask(&buffer,&length);
    unpackPropagateTask(buffer,length);

  }
  /*------------------------------*/
  void unpackPropagateTask(byte *buffer,int length){
    PropagateInfo P;
    int offset = 0 ;
    while (offset <length) {
      P.deserialize(buffer,offset,length);
      printf ("  @A(%d,%d)%ld-%ld [%s%d]== [%s0]\n",P.i,P.j,
	      P.data_handle.context_handle,
	      P.data_handle.data_handle,
	      P.fromCtx.c_str(),
	      P.fromVersion,P.toCtx.c_str());
      dtEngine.addPropagateTask(&P);
    }
  }
  /*------------------------------*/
  void packPropagateTask(list < PropagateInfo *> prop_info,int dest_host){
    list < PropagateInfo *>::iterator it;
    int msg_max_size = 1024; //prop_info.size() * sizeof(PropagateInfo) ;// todo
    int msg_len = 0;
    printf("gctx malloc sz:%d\n",msg_max_size);

    byte  *msg_buffer= (byte *)malloc ( msg_max_size);
    for (it = prop_info.begin(); it != prop_info.end();++it ) {
      PropagateInfo &P=*(*it);
      if ( dest_host != me ) {
	P.serialize(msg_buffer,  msg_len,msg_max_size);      
	dtEngine.sendPropagateTask(msg_buffer,msg_len,dest_host);
	incrementCounter(TaskPropagate);
	incrementCounter(PropagateSize,msg_len);
      }
      else{
	dtEngine.addPropagateTask(&P);
      }
    }
  }
  /*------------------------------*/
  void sendPropagateTasks(){
    list <PropagateInfo *>::iterator it;
    for ( it = lstPropTasks.begin(); it != lstPropTasks.end();++it){
      int p = getDataHostPolicy()->getHost(Coordinate ((*it)->i,(*it)->j));
      (*it)->toCtx = getLevelString();
      nodesPropTasks[p].push_back((*it));
    }
    for ( int p= 0; p < cfg->getProcessors(); p++){
      if (nodesPropTasks[p].size()==0) continue;
      printf ("  @send propagate to: %d \n",p) ;
      dumpPropagations(nodesPropTasks[p]);
      packPropagateTask(nodesPropTasks[p],p);
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
