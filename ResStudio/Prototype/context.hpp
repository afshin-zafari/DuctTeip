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
#include "engine.hpp"

using namespace std;

GlobalContext glbCtx;
int me,version;

class IContext;
class IData;
/*===============================  Context Class =======================================*/
class IContext
{
protected:
  string           name;
  IContext        *parent;
  list<IContext*>  children;
  list<IData*>     inputData,
                   outputData,
                   in_outData;
  ProcessGrid     *PG;
  ContextHandle   *my_context_handle;
public:
   IContext(string _name){
    name=_name;
    parent =NULL;
    setContextHandle ( glbCtx.createContextHandle() ) ;
    glbCtx.addContext( this ) ;
  }
  ~IContext(){
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      delete (*ctx_it);
    }
    inputData.clear();
    outputData.clear();
    children.clear();
  }
  virtual void runKernels(ITask *task) = 0 ; 
  virtual string getTaskName(unsigned long) =0;

          string getName()    {return name;}
          string getFullName(){return getName();}
          void   print_name       ( const char *s="") {cout << s << name << endl;}
          void   setContextHandle ( ContextHandle *c) {my_context_handle = c;}
  ContextHandle *getContextHandle ()                  { return my_context_handle;}

  IData *getDataFromList(list<IData* > dlist,int index){
    list<IData *>:: iterator it;
    if ( index >= dlist.size() ) 
      return NULL;
    it=dlist.begin();
    advance(it,index);
    return (*it);
  }
  IData *getOutputData  (int index=0){    
    return getDataFromList ( outputData , index);
  }
  IData *getInputData   (int index=0){    
    return getDataFromList ( inputData , index);
  }
  IData *getInOutData   (int index=0){    
    return getDataFromList ( in_outData , index);
  }
  
  
  IData *getDataByHandle(list<IData *> dlist,DataHandle *dh){
    list<IData*> :: iterator it;
    IData* data= new IData("",0,0,this);
    for (it = dlist.begin(); it != dlist.end();++it) {
      data = (*it)->getDataByHandle(dh);
      if ( data ->getName().size() != 0 ) 
	return data;
    }
    return data;
  }
  IData *getDataByHandle(DataHandle *dh ) {
    IData *data;
    data = getDataByHandle ( inputData,dh);
    if ( data ->getName().size() != 0 ) 
      return data;
    data = getDataByHandle ( outputData,dh);
    if ( data ->getName().size() != 0 ) 
      return data;
    data = getDataByHandle ( in_outData,dh);
    if ( data ->getName().size() != 0 ) 
      return data;
    return data;
  }

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
  void addInOutData(IData *_d){
    in_outData.push_back(_d);
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
  void testHandles(list<IData*> dlist){
    list<IData*>::iterator it ;
    for (it = dlist.begin(); it != dlist.end(); ++it) {
      (*it)->testHandles();
    }
  }
  void testHandles(){
    IContext * c = glbCtx.getContextByHandle(*my_context_handle);
    printf("Context:%s - Handle:%ld <--> %s,%ld\n",name.c_str(),*my_context_handle,c->getName().c_str(),*c->getContextHandle());
    printf("InputData Handles \n");
    testHandles( inputData);
    printf("Output Data Handles \n");
    testHandles(outputData);
    printf("In/Out Data  Handles \n");
    testHandles(in_outData);
  }

  DataHandle * createDataHandle ( ) {
    DataHandle *d =glbCtx.createDataHandle () ;
    d->context_handle = *my_context_handle ; 
    return d;
  }
 
};
/*===================================================================================*/
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
/*===================================================================================*/
       IData::IData            (string _name,int m, int n,IContext *ctx):    M(m),N(n), parent_context(ctx){
    name=_name;
     current_version.reset();
     request_version.reset();
     rt_read_version.reset();
    rt_write_version.reset();
     rt_read_version.setContext(glbCtx.getLevelString());
    rt_write_version.setContext(glbCtx.getLevelString());
    setDataHandle( ctx->createDataHandle());
}
bool   IData::isOwnedBy        (int p ) {
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
int    IData::getHost          (){
  Coordinate c = blk;  
  if (parent_data->Nb == 1 ||   parent_data->Mb == 1 ) {
    if ( parent_data->Nb ==1) 
      c.bx = c.by;
    return ( hpData->getHost ( c,1 )  );
  }
  else
    return hpData->getHost(blk);
}
void   IData::incrementVersion ( AccessType a) {
    current_version++;
    if ( a == WRITE ) {
      request_version = current_version;
    }
  }
IData *IData::getDataByHandle  (DataHandle *in_dh ) {
  ContextHandle *ch = parent_context->getContextHandle();
  IData *not_found= new IData("",0,0,parent_context);
  if ( *ch != in_dh->context_handle ){ 
    return not_found;
  }
  for ( int i = 0; i < Mb; i++) {
    for ( int j = 0; j< Nb; j++){
      DataHandle *my_dh = (*dataView)[i][j]->getDataHandle();
      if ( my_dh->data_handle == in_dh->data_handle ) {
	return  (*dataView)[i][j];
      }
    }
  }
  return not_found;
}
void   IData::testHandles      (){
  printf("Data:%s, Context:%ld Handle :%ld\n",getName().c_str(),my_data_handle->context_handle,my_data_handle->data_handle);
  for(int i=0;i<Mb;i++){
    for (int j=0;j<Nb;j++){
      DataHandle *dh = (*dataView)[i][j]->getDataHandle();
      IData *d = glbCtx.getDataByHandle(dh);
      if ( d->getName().size()!= 0 ) 
	printf("%s-%ld,%ld  <--> %s,%ld\n", (*dataView)[i][j]->getName().c_str(),dh->context_handle,dh->data_handle,
	       d->getName().c_str(),d->getDataHandle()->data_handle);
      else 
	printf("%s-%ld,%ld  <--> NULL  \n", (*dataView)[i][j]->getName().c_str(),dh->context_handle,dh->data_handle);
    }
  }
}
/*===================================================================================*/
IContext *GlobalContext::getContextByHandle(ContextHandle ch) {
  list<IContext *> ::iterator it;
  for ( it= children.begin(); it != children.end();++it)
    if (*(*it)->getContextHandle() == ch) 
	return *it;
  
}
IData    *GlobalContext::getDataByHandle   (DataHandle *d){
  IContext *ctx = getContextByHandle(d->context_handle);
  return ctx->getDataByHandle(d);
}

DataRange *GlobalContext::getIndependentData(IContext *ctx,int data_type){
    IData * data=NULL ;
    if ( data_type == InputData)
      data = ctx->getInputData(); // todo loop for all input data
    else if (data_type == OutputData)  
      data = ctx->getOutputData();// todo loop for all output data
    else if (data_type == InOutData)
      data = ctx->getInOutData(); // todo loop for all inout data
    if ( !data ) 
      return NULL;
    ContextHandle dch = *data->getParent()->getContextHandle();
    ContextHandle cch = *ctx->getContextHandle();
    if ( dch == cch ) 
      return data->All();
    return NULL;
  }

void      GlobalContext::dumpStatistics    (Config *_cfg){
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
void      GlobalContext::setConfiguration  (Config *_cfg){
  cfg=_cfg;
  list < PropagateInfo *> lp;
  for ( int i = 0 ; i <cfg->getProcessors();i++)
    nodesPropTasks.push_back(lp);
}
void      GlobalContext::testHandles       (){
  list<IContext *>::iterator it;
  for(it = children.begin();it != children.end(); ++it){
    (*it)->testHandles();      
  }
}
/*===================================================================================*/
void PropagateInfo::dump(){
    IData *data  = glbCtx.getDataByHandle(&data_handle);
    printf("prop: %s [%s%d] -> [%s0]\n",data->getName().c_str(),fromCtx.c_str(),fromVersion,toCtx.c_str());
  }

/*===================================================================================*/
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
bool ContextHostPolicy::isAllowed(IContext *ctx,ContextHeader *hdr){
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
  if ( gc  == DONT_CARE_COUNTER && (active_policy != WRITE_DATA_OWNER)  )
    return true;
  if ( active_policy  == ROOT_ONLY ) {
    return ( me == 0 ) ;
  }
  
  if ( active_policy ==NOT_OWNER_CYCLIC || active_policy == WRITE_DATA_OWNER) {
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
    //printf("No\n");
    if (active_policy == WRITE_DATA_OWNER) 
      return false;
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
    //printf("taskprop policy : pcnt:%d grp:%d lower:%d me:%d is_allowed:%d\n",propagate_count ,group_size  ,lower , me,b);
    propagate_count++;
    return b;
    
  }
  return false;
}
/*===================================================================================*/
bool begin_context(IContext * curCtx,DataRange *r1,DataRange *r2,DataRange *w,int counter,int from,int to){
  static int count=0;
  ContextHeader *Summary = glbCtx.getHeader();
  Summary->clear();
  Summary->addDataRange(IData::READ,r1);
  if ( r2 != NULL )
    Summary->addDataRange(IData::READ,r2);
  Summary->addDataRange(IData::WRITE,w);
  printf("@BeginContext \n");
  printf("@change Context(beg) from:%8.8s* %d\n",glbCtx.getLevelString().c_str(),count++);
  glbCtx.getContextHostPolicy()->setGroupCounter(counter);
  //glbCtx.sendPropagateTasks();
  // glbCtx.createPropagateTasks();
  glbCtx.downLevel();
  glbCtx.beginContext();
  glbCtx.sendPropagateTasks();
  glbCtx.resetVersiosOfHeaderData();
  printf("@change Context(beg) to  :%8.8s* %d\n",glbCtx.getLevelString().c_str(),count);
  bool b=glbCtx.getContextHostPolicy()->isAllowed(curCtx,Summary);
  if (b){
    glbCtx.incrementCounter(GlobalContext::EnterContexts);
  }
  else{
    //curCtx->dumpDataVersions(Summary);
    glbCtx.incrementCounter(GlobalContext::SkipContexts);
  }
  return b;
}
void end_context(IContext * curCtx){
  static int count=0;
  printf("@EndContext \n");
  printf("@change Context(end) from:%8.8s* %d\n",glbCtx.getLevelString().c_str(),count++);
  glbCtx.createPropagateTasks();
  glbCtx.upLevel();
  //glbCtx.sendPropagateTasks();
  //glbCtx.resetVersiosOfHeaderData();
  printf("@change Context(end) to  :%8.8s* %d\n",glbCtx.getLevelString().c_str(),count);
  glbCtx.endContext();
  glbCtx.getHeader()->clear();
}
void AddTask ( IContext *ctx,char*s,unsigned long key,IData *d1,IData *d2,IData *d3){
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
    list<DataAccess *> *dlist = new list <DataAccess *>;
    DataAccess *daxs ;
    if ( d1 != NULL ) {
      daxs = new DataAccess;
      daxs->data = d1;
      daxs->required_version = d1->getRequestVersion();
      daxs->required_version.setContext( glbCtx.getLevelString() );
      d1->getRequestVersion().setContext( glbCtx.getLevelString() );
      d1->getCurrentVersion().setContext( glbCtx.getLevelString() );
      daxs->type = IData::READ;
      dlist->push_back(daxs);
    }
    if ( d2 != NULL ) {
      daxs = new DataAccess;
      daxs->data = d2;
      daxs->required_version = d2->getRequestVersion();
      daxs->required_version.setContext( glbCtx.getLevelString() );
      d2->getRequestVersion().setContext( glbCtx.getLevelString() );
      d2->getCurrentVersion().setContext( glbCtx.getLevelString() );
      daxs->type = IData::READ;
      dlist->push_back(daxs);
    }
    daxs = new DataAccess;
    daxs->data = d3;
    daxs->required_version = d3->getCurrentVersion();
    daxs->required_version.setContext( glbCtx.getLevelString() );
    d3->getRequestVersion().setContext( glbCtx.getLevelString() );
    d3->getCurrentVersion().setContext( glbCtx.getLevelString() );
    daxs->type = IData::WRITE;
    dlist->push_back(daxs);

    TaskHandle task_handle =dtEngine.addTask(ctx,s,key,d3->getHost(),dlist);
    printf (" @Insert TASK:%s(%ld)  %s,host=%d\n", s,task_handle,d3->isOwnedBy(me)?"*":"",me);
    if (d1) d1->dump();
    if (d2) d2->dump();
    d3->dump();
  }
  else{
    printf (" @Skip TASK:%s  %s,host=%d\n", s,d3->isOwnedBy(me)?"*":"",me);
    if (d1) d1->dump();
    if (d2) d2->dump();
    d3->dump();
  }

  if ( d1 != NULL ) d1->incrementVersion(IData::READ);
  if ( d2 != NULL ) d2->incrementVersion(IData::READ);
  d3->incrementVersion(IData::WRITE);

  /*
  printf("  @after-exec: \n");
  if (d1) d1->dump();
  if (d2) d2->dump();
  d3->dump();
  */

  delete dr;
  c->clear();
  delete c;
}
void AddTask ( IContext *ctx,char*s,unsigned long key,IData *d1                    ) { AddTask ( ctx,s,key,NULL,NULL , d1);}
void AddTask ( IContext *ctx,char*s,unsigned long key,IData *d1,IData *d2          ) { AddTask ( ctx,s,key,d1  ,NULL , d2);}

/*===============================================================================*/
void ITask::dump(){
    printf("#task:%s key:%lx ,no.of data:%ld state:%d\n ",name.c_str(),key,data_list->size(),state);
    if (type == PropagateTask)
      prop_info->dump();
    else
      dumpDataAccess(data_list);
  }
ITask::ITask(PropagateInfo *p){//todo
  prop_info = p;
  type = PropagateTask;
  data_list = new list<DataAccess*>;
  DataAccess *daxs = new DataAccess;
  daxs->data = glbCtx.getDataByHandle(&p->data_handle);  
  daxs->required_version = p->fromVersion;
  daxs->required_version.setContext( p->fromCtx);
  daxs->type = IData::READ;
  data_list->push_back(daxs);  
}
 void ITask::runPropagateTask(){
  IData *data = glbCtx.getDataByHandle(&prop_info->data_handle);
  data->setRunTimeVersion(prop_info->toCtx,0);
  setFinished(true);
}

int ITask::serialize(byte *buffer,int &offset,int max_length){
    int count =data_list->size();
    list<DataAccess *>::iterator it;
    copy<unsigned long>(buffer,offset,key);
    ContextHandle handle=*parent_context->getContextHandle();
    copy<ContextHandle>(buffer,offset,handle);
    copy<int>(buffer,offset,count);
    for ( it = data_list->begin(); it != data_list->end(); ++it ) {
      (*it)->data->getDataHandle()->serialize(buffer,offset,max_length);
      (*it)->required_version.serialize(buffer,offset,max_length);
      byte type = (*it)->type;
      copy<byte>(buffer,offset,type);
    }
}

void ITask::deserialize(byte *buffer,int &offset,int max_length){
  paste<unsigned long>(buffer,offset,&key);
  size_t len = sizeof(unsigned long);
  ContextHandle handle ;
  paste<ContextHandle>(buffer,offset,&handle);
  parent_context = glbCtx.getContextByHandle(handle);
  name=parent_context->getTaskName(key);
  int count ;
  paste<int>(buffer,offset,&count);
  data_list = new list<DataAccess *>;
  for ( int i=0; i<count; i++){
    DataAccess *data_access = new DataAccess;
    DataHandle *data_handle = new DataHandle;
    data_handle->deserialize(buffer,offset,max_length);
    IData *data = glbCtx.getDataByHandle(data_handle);
    data_access->required_version.deserialize(buffer,offset,max_length);
    paste<byte>(buffer,offset,&data_access->type);
    data_access->data = data;
    data_list->push_back(data_access);
  }
}
void ITask::run(){
    if ( state == Finished ) 
      return;
    if (type == PropagateTask){
      runPropagateTask();
    }
    else
      parent_context->runKernels(this);
}
void ITask::setFinished(bool flag){
    list<DataAccess *>::iterator it;
    if ( !flag ) 
      return ;
    state = Finished;
    if ( type == PropagateTask ) 
      return;
    for ( it = data_list->begin(); it != data_list->end(); ++it ) {
      printf("** data upgraded :%s\n",(*it)->data->getName().c_str());
      (*it)->data->incrementRunTimeVersion((*it)->type);
    }
    
  }
/*===============================================================================*/
void IListener::deserialize(byte *buffer, int &offset, int max_length){
    DataAccess *data_access = new DataAccess;
    DataHandle *data_handle = new DataHandle;
    paste<int>(buffer,offset,&host);
    data_handle->deserialize(buffer,offset,max_length);
    IData *data = glbCtx.getDataByHandle(data_handle);
    data_access->required_version.deserialize(buffer,offset,max_length);
    data_access->data = data;
    data_request = data_access;
  }
/*===============================================================================*/
void engine::receivePropagateTask(byte *buffer, int len){
    PropagateInfo *p = new PropagateInfo;
    int offset =0 ;
    p->deserialize(buffer,offset,len);
    addPropagateTask(p);
  }

void engine::addPropagateTask(PropagateInfo *P){//todo 
  ITask *task = new ITask(P);
  string s("PROPTASK");
  task->setName(s);
  criticalSection(Enter) ;
  TaskHandle task_handle = last_task_handle ++;
  task->setHandle(task_handle);
  task_list.push_back(task);
  putWorkForPropagateTask(task);
  criticalSection(Leave) ;
}
void engine:: sendTask(ITask* task,int destination){
  int offset = 0 ;
  int msg_size = task->getSerializeRequiredSpace();
  byte *buffer = (byte *)malloc(msg_size);

  task->serialize(buffer,offset,msg_size);
  unsigned long ch =  mailbox->send(buffer,offset,MailBox::TaskTag,destination);
  task->setCommHandle (ch);
  
}
/*=====================================================================*/
void engine::receivedData(MailBoxEvent *event){
    IData *temp_data = new IData;
    int offset =0 ;
    temp_data->deserialize(event->buffer,offset,event->length);
    IData *data = glbCtx.getDataByHandle(temp_data->getDataHandle());
    offset =0 ;
    data->deserialize(event->buffer,offset,event->length,false);
    delete temp_data;
  }


#endif //  __CONTEXT_HPP__
