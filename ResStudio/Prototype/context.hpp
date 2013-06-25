#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <string>
#include <cstdio>

#include <vector>
#include <list>

#include "config.hpp"
#include "procgrid.hpp"
#include "hostpolicy.hpp"
#include "data.hpp"
#include "glb_context.hpp"

using namespace std;

GlobalContext glbCtx;
int me,version;

class IContext;
class IData;
/*===============================  Context Class =======================================*/
class IContext
{
protected:
  string name;
  IContext *parent;
  list<IContext*> children;
  list<IData*> inputData,outputData;
  ProcessGrid *PG;
public:
  IContext(string _name){
    name=_name;
    parent =NULL;
  }
  string getName(){return name;}
  string getFullName(){return getName();}
  void print_name(const char *s=""){cout << s << name << endl;}
  virtual void downLevel(){}
  virtual void upLevel(){}
  
  IData *getDataFromList(list<IData* > dlist,int index){
    list<IData *>:: iterator it;
    it=dlist.begin();
    advance(it,index);
    return (*it);
  }
  IData *getOutputData(int index=0){    
    return getDataFromList ( outputData , index);
  }
  IData *getInputData(int index=0){    
    return getDataFromList ( inputData , index);
  }

  void createPropagateTasks(){}
  void setParent ( IContext *_p ) {
    parent = _p;
    parent->children.push_back(this);
  }
  void traverse(){
    if ( parent != NULL )
      parent->print_name("-");
    else
      cout << "-" << endl;
    print_name("--");
    for (list<IContext *>::iterator it=children.begin();it !=children.end();it++)
      (*it)->print_name("---");
  }
  void addInputData(IData *_d){
    inputData.push_back(_d);
  }
  void addOutputData(IData *_d){
    outputData.push_back(_d);
  }
  ~IContext(){
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    for ( it =inputData.begin(); it != inputData.end(); ++ it ){
      //delete (*it);
    }
    /*
    for ( it =outputData.begin(); it != outputData.end(); ++ it ){
      delete (*it);
    }
    */
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      delete (*ctx_it);
    }
    inputData.clear();
    outputData.clear();
    children.clear();
  }
  void resetVersions(){
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    for ( it =inputData.begin(); it != inputData.end(); ++ it ){
      (*it)->resetVersion();
    }
    /*
    for ( it =outputData.begin(); it != outputData.end(); ++ it ){
      (*it)->resetVersion();
    }
    */
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      (*ctx_it)->resetVersions();

    }
  }
  void dumpDataVersions(ContextHeader *hdr=NULL){
    list<IData*>::iterator it ;
    if ( hdr == NULL ) {
      glbCtx.dumpLevels();
      for ( it =inputData.begin(); it != inputData.end(); ++ it ){
	(*it)->dumpVersion();
      }
    }
    else {
      list <DataRange *> dr = hdr->getWriteRange();
      list<DataRange*>::iterator  it = dr.begin();
      for ( ; it != dr.end() ;++it ){
	for ( int r = (*it)->row_from; r <=(*it)->row_to; r++){
	  for ( int c = (*it)->col_from; c <=(*it)->col_to; c++){
	    IData &d=*((*it)->d);
	    d(r,c)->dumpVersion();
	  }
	}
      }
      dr = hdr->getReadRange();

      for (it = dr.begin() ; it != dr.end() ;++it ){
	for ( int r = (*it)->row_from; r <=(*it)->row_to; r++){
	  for ( int c = (*it)->col_from; c <=(*it)->col_to; c++){
	    IData &d=*((*it)->d);
	    d(r,c)->dumpVersion();
	  }
	}
      }
    }

  }
};
/*=======================================================================*/

class Context: public IContext
{
public :
  Context (const char *s ) :
  IContext(static_cast<string>(s))
  {
  }
  Context (void):IContext("") {
    name="";
  }
};

/*=======================================================================*/
bool IData::isOwnedBy(int p ) {
  Coordinate c = blk;
  
  
  if (parent_data->Nb == 1 ||   parent_data->Mb == 1 ) {
    if ( parent_data->Nb ==1) 
      c.bx = c.by;
    return ( hpData->getHost ( c,1 ) == p );
  }
  else{
    bool b = (hpData->getHost ( blk ) == p) ;
    return b;
  }
}
int IData::getHost(){
  return hpData->getHost(blk);
  }

void IData::incrementVersion ( AccessType a) {
    DataVersions v;
    current_version++;
    if ( a == WRITE ) {
      request_version = current_version;
    }
  }
/*=======================================================================*/

void GlobalContext::dumpStatistics(Config *_cfg){
  if ( me ==0)printf("#STAT:Node\tCtxIn\tCtxSkip\tT.Read\tT.Ins\tT.Prop.\tP.Size\tComm\tTotTask\tNb\tP\n");
  printf("#STAT:%2d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
	 me,
	 counters[EnterContexts],
	 counters[SkipContexts],
	 counters[TaskRead],
	 counters[TaskInsert],
	 counters[TaskPropagate],
	 counters[PropagateSize],
	 counters[CommCost],
	 TaskCount,
	 _cfg->getXBlocks(),
	 _cfg->getProcessors());
}
void GlobalContext::setConfiguration(Config *_cfg){
  cfg=_cfg;
  list < PropagateInfo *> lp;
  for ( int i = 0 ; i <cfg->getProcessors();i++)
    nodesPropTasks.push_back(lp);
}

/*=============================================================================*/

/*----------------------------------------------------------------------*/
/*
 * At every level of contexts, processors are partitioned into sub-groups.
 * interval [offset + a,offset + b] :
 *   contains host id's of allowed group
 * s :
 *   size of each partition
 * p :
 *   index of allowed partition
 * groupCount[level]:
 *   number of partitions within the 'level'
 */
/*----------------------------------------------------------------------*/
bool ContextHostPolicy ::isAllowed(IContext *ctx,ContextHeader *hdr){
  if (active_policy == PROC_GROUP_CYCLIC){
    if (groupCounter  != DONT_CARE_COUNTER ) {
      int lower,upper;
      getHostRange(&lower,&upper);
      int group_size = upper - lower +1;
      int level = glbCtx.getDepth();
      bool b = ((groupCounter % group_size ) + lower ) == me;
      //if(b) printf("lv=%d,[%d %d],  gc=%d,gs=%d,allowed?%d\n",level,lower,upper,groupCounter,group_size,b);
      return b;
    }
    int a = 0 ;
    int b = PG->getProcessorCount()-1;
    int offset = 0 ;
    int s;
    int p;
    for ( int level=0; level<glbCtx.getDepth();level++){
      s = (b - a +1 )  / groupCount[level];
      p = glbCtx.getLevelID(level) % groupCount[level];
      a = p * s;
      b = (p+1) *s -1 ;
      if (me < (offset + a) || me > (offset+b) )  {
	//printf("ContextHostPolicy: %d is NOT in [%d,%d]\n",me,offset+a,offset+b);
	return false;
      }
      offset  += a;
    }
    if (me >= (offset) && me <= (offset-a+b) ) {
      //printf("ContextHostPolicy: %d IS in [%d,%d]\n",me,offset+a,offset+b);
      return true;
    }
    //printf("ContextHostPolicy: %d is NOT in [%d,%d]\n",me,offset+a,offset+b);
  }
  return false;
}

/*----------------------------------------------------------------------*/
void ContextHostPolicy::getHostRange(int *lower, int *upper){
  if (active_policy == PROC_GROUP_CYCLIC){
    int a = 0 ;
    int b = PG->getProcessorCount()-1;
    int offset = 0 ;
    int s;
    int p;
    for ( int level=0; level<glbCtx.getDepth();level++){
      s = (b - a +1 )  / groupCount[level];
      p = glbCtx.getLevelID(level) % groupCount[level];
      a = p * s;
      b = (p+1) *s -1 ;
      offset  += a;
    }
    *lower = offset;
    *upper = offset -a + b;
  }
}

/*===================================================================================*/
bool TaskReadPolicy::isAllowed(IContext *c,ContextHeader *hdr){
  if ( active_policy == ALL_GROUP_MEMBERS ) {
    return true;
  }
  return false;
}
/*===================================================================================*/
bool TaskAddPolicy::isAllowed(IContext *c,ContextHeader *hdr){
  int gc = glbCtx.getContextHostPolicy()->getGroupCounter();
  printf("add task policy  1,%d\n",gc);

  if ( gc  == DONT_CARE_COUNTER )  
    return true;
  printf("add task policy  2\n");
  if ( active_policy  == ROOT_ONLY ) {
    //printf("ROOT_ONLY policy me=%d,me==0?%d\n",me,me==0);
    return ( me == 0 ) ;
  }
  
  printf("add task policy  3\n");
  if ( active_policy ==NOT_OWNER_CYCLIC ) {
    int r,c;
    IData *A;
    list<DataRange *> dr = hdr->getWriteRange();
    list<DataRange *>::iterator it;
    //printf("TaskAddPolicy: isAllowed(%d)?\n",me);
    for ( it = dr.begin(); it != dr.end(); ++it ) {
      for (  r=(*it)->row_from; r<= (*it)->row_to;r++){
	for (  c=(*it)->col_from; c<= (*it)->col_to;c++){
	  A=(*it)->d;
	  //printf("A(%d,%d), %s\n",r,c,(*A)(r,c)->getName().c_str());
	  if ( (*A)(r,c)->isOwnedBy(me) ) {
	    //printf("Yes\n");
	    return true;
	  }
	}
      }
    }

    //printf("No.\n");
    it = dr.begin();
    r=(*it)->row_from;
    c=(*it)->col_from;
    A=(*it)->d;
    int owner = (*A)(r,c)->getHost () ;

    ContextHostPolicy *chp = static_cast<ContextHostPolicy *>(glbCtx.getContextHostPolicy());
    int lower,upper;
    chp->getHostRange(&lower,&upper);
      if ( owner >= lower && owner <= upper && owner != me)
	return false;
    int group_size = upper - lower + 1;
    bool b = ((not_owner_count % group_size ) + lower ) == me;
    //    printf("Not Owner, [%d %d] g-size=%d, me=%d,nocnt=%d,allowed=%d\n",
    //	  lower,upper,group_size,me,not_owner_count, b);
    not_owner_count++;
    return b;
  }
  return false;
}
/*===================================================================================*/
bool TaskPropagatePolicy::isAllowed(ContextHostPolicy *hpContext,int me){
  int lower,upper;
  hpContext->getHostRange(&lower,&upper);
  if (active_policy == GROUP_LEADER )
    return ( me == lower);
  if (active_policy == ALL_CYCLIC) {
    int group_size = upper - lower + 1;
    bool b = ((propagate_count % group_size ) + lower ) == me;
    propagate_count++;
    return b;
    
  }
  return false;
}
/*===================================================================================*/
bool begin_context(IContext * curCtx,DataRange *r1,DataRange *r2,DataRange *w,int counter,int from,int to){
  ContextHeader *Summary = glbCtx.getHeader();
  Summary->clear();
  Summary->addDataRange(IData::READ,r1);
  if ( r2 != NULL )
    Summary->addDataRange(IData::READ,r2);
  Summary->addDataRange(IData::WRITE,w);
  glbCtx.getContextHostPolicy()->setGroupCounter(counter);
  glbCtx.downLevel();
  glbCtx.beginContext();
  glbCtx.sendPropagateTasks();
  printf("@BeginContext %s,%d\n",glbCtx.getLevelString().c_str(), glbCtx.getID());
  bool b=glbCtx.getContextHostPolicy()->isAllowed(curCtx,Summary);
  if (b){
    glbCtx.setHeaderRange(0,0);
    glbCtx.incrementCounter(GlobalContext::EnterContexts);
  }
  else{
    //curCtx->dumpDataVersions(Summary);
    glbCtx.setHeaderRange(from,to);
    glbCtx.incrementCounter(GlobalContext::SkipContexts);
  }
  return b;
}

void end_context(IContext * curCtx){
  glbCtx.createPropagateTasks();
  glbCtx.upLevel();
  printf("@EndContext %s\n",glbCtx.getLevelString().c_str());
  glbCtx.endContext();
  glbCtx.getHeader()->clear();
}

void AddTask(IContext *ctx,char*s,IData *d1,IData *d2,IData *d3)
{
  ContextHeader *c=NULL;
  if ( !glbCtx.getTaskReadHostPolicy()->isAllowed(ctx,c) )
    return;

  glbCtx.incrementCounter(GlobalContext::TaskRead);

  DataRange * dr = new DataRange;
  dr->d = d3->getParentData();
  Coordinate b = d3->getBlockIdx();
  dr->row_from = dr->row_to = b.by;
  dr->col_from = dr->col_to = b.bx;
  c = new ContextHeader ;
  c->addDataRange(IData::WRITE,dr);

  if ( glbCtx.getTaskAddHostPolicy()->isAllowed(ctx,c) ) {
    glbCtx.incrementCounter(GlobalContext::TaskInsert);
    if ( !d3->isOwnedBy(me) ){
      glbCtx.incrementCounter(GlobalContext::CommCost);
    }
    
    printf (" @Insert TASK:%s %s,%d %s,%d %s,%d %s,host=%d\n", s,
	    (d1==NULL)?"  ---- ":d1->getName().c_str(),	 (d1==NULL) ?-1:d1->getRequestVersion(),
	    (d2==NULL)?"  ---- ":d2->getName().c_str(),	 (d2==NULL) ?-1:d2->getRequestVersion(),
	    d3->getName().c_str(),	 d3->getCurrentVersion(),
	    d3->isOwnedBy(me)?"*":"",
	    me
      );
    printf("  @after-exec:        %d       %d      %d \n",
	   (d1==NULL) ?-1:d1->getCurrentVersion(),
	   (d2==NULL) ?-1:d2->getCurrentVersion(),
	   d3->getCurrentVersion()
	   );
  }

  if ( d1 != NULL ) d1->incrementVersion(IData::READ);
  if ( d2 != NULL ) d2->incrementVersion(IData::READ);
  d3->incrementVersion(IData::WRITE);

  delete dr;
  c->clear();
  delete c;



}
void AddTask ( IContext *c,char*s,IData *d1)           { AddTask ( c,s,NULL,NULL , d1);}
void AddTask ( IContext *c,char*s,IData *d1,IData *d2) { AddTask ( c,s,d1,NULL , d2);}


#endif //  __CONTEXT_HPP__
