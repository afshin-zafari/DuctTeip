
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
engine dtEngine;

int me,version,simulation;

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
  Config *cfg;
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
  virtual void runKernels(IDuctteipTask *task) = 0 ; 
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
    IData* data;
    for (it = dlist.begin(); it != dlist.end();++it) {
      data = (*it)->getDataByHandle(dh);
      if ( data ) 
	if ( data ->getName().size() != 0 ) 
	  return data;
    }
    return NULL;
  }
  IData *getDataByHandle(DataHandle *dh ) {
    IData *data=NULL;
    data = getDataByHandle ( inputData,dh);
    if ( data ) 
      if ( data ->getName().size() != 0 ) 
	return data;
    data = getDataByHandle ( outputData,dh);
    if ( data ) 
      if ( data ->getName().size() != 0 ) 
	return data;
    data = getDataByHandle ( in_outData,dh);
    if ( data ) 
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
    if(0)printf("@data se dh:%ld\n",d->data_handle);
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
void IData::prepareMemory(){
    
  //      allocateMemory();
      dtPartition=new Partition<double>(2);
      Partition<double> *p = dtPartition;
      p->setBaseMemory(getContentAddress() ,  getContentSize());
      if(0)
	printf("####%s, PrepareData cntntAdr:%ld sz:%d\n",getName().c_str(),
	       getContentAddress(),getContentSize());
      p->partitionRectangle(local_m,local_n,local_mb,local_nb);	

}
void IData::setPartition(int _mb, int _nb){
    
  Nb = _nb;
  Mb = _mb;
  int i_ex=0,j_ex=0;
  partial = false;
  if (0) {
    if (N % Nb !=0){
      partial=true;
      j_ex=1;
    }
    if (M % Mb !=0){
      partial=true;
      i_ex=1;
    }
  }
  dataView=new vector<vector<IData*> >  (Mb+i_ex, vector<IData*>(Nb+j_ex)  );
  char s[100];
  for ( int i=0;i<(Mb+i_ex);i++)
    for ( int j=0;j<(Nb+j_ex);j++){
      addLogEventStart("DataPartitioned",DuctteipLog::DataPartitioned);
      sprintf(s,"%s_%2.2d_%2.2d",  name.c_str() , i ,j);
      if ( Nb == 1) sprintf(s,"%s_%2.2d",  name.c_str() , i );
      if ( Mb == 1) sprintf(s,"%s_%2.2d",  name.c_str() , j );
      (*dataView)[i][j] = new IData (static_cast<string>(s),M/Mb,N/Nb,parent_context);
      IData *newPart = (*dataView)[i][j];
      newPart->blk.bx = j;
      newPart->blk.by = i;
      newPart->parent_data = this ;
      newPart->hpData = hpData ;
      newPart->Nb = 0 ;
      newPart->Mb = 0 ;
      newPart->N  = N ;
      newPart->M  = M ;
      newPart->local_nb  = local_nb ;
      newPart->local_mb  = local_mb ;
      newPart->local_n = N / Nb   ;
      newPart->local_m = M / Mb   ;
      if ( newPart->getHost() == me ) {
	newPart->allocateMemory();
	newPart->dtPartition=new Partition<double>(2);
	Partition<double> *p = newPart->dtPartition;
	p->setBaseMemory(newPart->getContentAddress() ,  newPart->getContentSize());
	if(0)
	  printf("AllocData for %s=%p sz:%d,N=%d,M=%d\n",s,newPart->getContentAddress(),newPart->getContentSize(),N,M);
	if (partial){
	  if(i == Mb && j == Nb){
	      printf("Data %s, has %d,%d elems.\n",s,M%Mb,N%Nb);
	      p->partitionRectangle(M % Mb, N%Nb,1,1);
	  }
	  else{
	    if(i == Mb ){
	      printf("Data %s, has %d,%d elems.\n",s,M%Mb,newPart->local_n);
	      p->partitionRectangle(M % Mb, newPart->local_n,1,1);
	    }
	    if ( j== Nb){
	      printf("Data %s, has %d,%d elems.\n",s,newPart->local_n,N%Nb);
	      p->partitionRectangle(newPart->local_m, N % Nb,1,1);
	    }
	  }
	}
	else
	  p->partitionRectangle(newPart->local_m,newPart->local_n,
				local_mb,local_nb);	
      }
      else{
	newPart->setRunTimeVersion("-1",-1);
	newPart->resetVersion();
      }
      addLogEventEnd("DataPartitioned",DuctteipLog::DataPartitioned);

    }
}
/*--------------------------------------------------------------*/
IData::IData(string _name,int m, int n,IContext *ctx):    
  M(m),N(n), parent_context(ctx)
{
  dt_log.addEventStart ( this,DuctteipLog::DataDefined);
  name=_name;
  gt_read_version.reset();
  gt_write_version.reset();
  rt_read_version.reset();
  rt_write_version.reset();
  string s = glbCtx.getLevelString();
  rt_read_version.setContext(s);
  rt_write_version.setContext(s);
  Mb = -1;Nb=-1;
  setDataHandle( ctx->createDataHandle());
  if(0) printf("@data se %s,dh:%ld\n",getName().c_str(),getDataHandleID());
  dt_log.addEventEnd ( this,DuctteipLog::DataDefined);
  hM = NULL;
  dtPartition = NULL;
  data_memory=NULL;
}
/*--------------------------------------------------------------*/
bool IData::isOwnedBy(int p) {
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
/*--------------------------------------------------------------*/
int IData::getHost(){
  Coordinate c = blk;  
  if (parent_data->Nb == 1 ||   parent_data->Mb == 1 ) {
    if ( parent_data->Nb ==1) 
      c.bx = c.by;
    return ( hpData->getHost ( c,1 )  );
  }
  else
    return hpData->getHost(blk);
}
/*--------------------------------------------------------------*/
void   IData::incrementVersion ( AccessType a) {
  gt_read_version++;
  if ( a == WRITE ) {
    gt_write_version = gt_read_version;
  }
}
/*--------------------------------------------------------------*/
IData *IData::getDataByHandle  (DataHandle *in_dh ) {
  ContextHandle *ch = parent_context->getContextHandle();
  IData *not_found=NULL;
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
/*--------------------------------------------------------------*/
void   IData::testHandles      (){
  printf("Data:%s, Context:%ld Handle :%ld\n",
	 getName().c_str(),my_data_handle->context_handle,my_data_handle->data_handle);
  for(int i=0;i<Mb;i++){
    for (int j=0;j<Nb;j++){
      DataHandle *dh = (*dataView)[i][j]->getDataHandle();
      IData *d = glbCtx.getDataByHandle(dh);
      if ( d->getName().size()!= 0 ) 
	printf("%s-%ld,%ld  <--> %s,%ld\n", 
	       (*dataView)[i][j]->getName().c_str(),dh->context_handle,dh->data_handle,
	       d->getName().c_str(),d->getDataHandle()->data_handle);
      else 
	printf("%s-%ld,%ld  <--> NULL  \n", 
	       (*dataView)[i][j]->getName().c_str(),dh->context_handle,dh->data_handle);
    }
  }
}
/*===================================================================================*/
IContext *GlobalContext::getContextByHandle(ContextHandle ch) {
  list<IContext *> ::iterator it;
  for ( it= children.begin(); it != children.end();++it)
    if (*(*it)->getContextHandle() == ch) 
      return *it;
  return NULL;
}
/*--------------------------------------------------------------*/
IData *GlobalContext::getDataByHandle(DataHandle *d){
  IContext *ctx = getContextByHandle(d->context_handle);
  return ctx->getDataByHandle(d);
}
/*--------------------------------------------------------------*/
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

/*--------------------------------------------------------------*/
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
/*--------------------------------------------------------------*/
void GlobalContext::setConfiguration(Config *_cfg){
  cfg=_cfg;
  list < PropagateInfo *> lp;
  for ( int i = 0 ; i <cfg->getProcessors();i++)
    nodesPropTasks.push_back(lp);
}
/*--------------------------------------------------------------*/
void GlobalContext::testHandles(){
  list<IContext *>::iterator it;
  for(it = children.begin();it != children.end(); ++it){
    (*it)->testHandles();      
  }
}
/*===================================================================================*/
void PropagateInfo::dump(){
  IData *data  = glbCtx.getDataByHandle(&data_handle);
  printf("prop: %s [%s%d] -> [%s0]\n",
	 data->getName().c_str(),fromCtx.c_str(),fromVersion,toCtx.c_str());
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
/*--------------------------------------------------------------*/
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
  if ( active_policy == ALL_READ_ALL ) {
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
  if (glbCtx.canAllEnter()){
    return true;
  }
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
/*--------------------------------------------------------------*/
void end_context(IContext * curCtx){
  static int count=0;
  if (glbCtx.canAllEnter()){
    return ;
  }
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
/*----------------------------------------------------------------------------*/
void AddTask ( IContext *ctx,
	       char*s,
	       unsigned long key,
	       IData *d1,
	       IData *d2,
	       IData *d3){
  ContextHeader *c=NULL;
  if ( !glbCtx.getTaskReadHostPolicy()->isAllowed(ctx,c) ){
    printf("ctx read task disallowed.\n");
    dt_log.addEventEnd("ReadTask",DuctteipLog::ReadTask);
    return;
  }

  glbCtx.incrementCounter(GlobalContext::TaskRead);

  DataRange * dr = new DataRange;
  dr->d = d3->getParentData();
  Coordinate b = d3->getBlockIdx();
  dr->row_from = dr->row_to = b.by;
  dr->col_from = dr->col_to = b.bx;
  c = new ContextHeader ;
  c->addDataRange(IData::WRITE,dr);
  dt_log.addEventStart("ReadTask",DuctteipLog::ReadTask);

  

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
      daxs->required_version = d1->getWriteVersion();
      daxs->required_version.setContext( glbCtx.getLevelString() );
      d1->getWriteVersion().setContext( glbCtx.getLevelString() );
      d1->getReadVersion().setContext( glbCtx.getLevelString() );
      daxs->type = IData::READ;
      dlist->push_back(daxs);
    }
    if ( d2 != NULL ) {
      daxs = new DataAccess;
      daxs->data = d2;
      daxs->required_version = d2->getWriteVersion();
      daxs->required_version.setContext( glbCtx.getLevelString() );
      d2->getWriteVersion().setContext( glbCtx.getLevelString() );
      d2->getReadVersion().setContext( glbCtx.getLevelString() );
      daxs->type = IData::READ;
      dlist->push_back(daxs);
    }
    daxs = new DataAccess;
    daxs->data = d3;
    daxs->required_version = d3->getReadVersion();
    daxs->required_version.setContext( glbCtx.getLevelString() );
    d3->getWriteVersion().setContext( glbCtx.getLevelString() );
    d3->getReadVersion().setContext( glbCtx.getLevelString() );
    daxs->type = IData::WRITE;
    dlist->push_back(daxs);

    TaskHandle task_handle =dtEngine.addTask(ctx,s,key,d3->getHost(),dlist);
    
    PRINT_IF(INSERT_TASK_FLAG) (" @Insert TASK:%s(%ld)  %s,host=%d\n", s,task_handle,d3->isOwnedBy(me)?"*":"",me);
    if (d1) d1->dump(' ');
    if (d2) d2->dump(' ');
    d3->dump(' ');
    
  }
  else{
    PRINT_IF(SKIP_TASK_FLAG)(" @Skip TASK:%s  %s,host=%d\n", s,d3->isOwnedBy(me)?"*":"",me);
  }
  /*  
  if ( d1 != NULL) 
    if ( d1->getHost() == me) {
      dtEngine.willReceiveListener(d1,d3->getHost(),d1->getReadVersion());
    }
  if ( d2 != NULL) 
    if ( d2->getHost() == me) {
      dtEngine.willReceiveListener(d2,d3->getHost(),d2->getReadVersion());
    }
  */
  if ( d1 != NULL ) d1->incrementVersion(IData::READ);
  if ( d2 != NULL ) d2->incrementVersion(IData::READ);
  d3->incrementVersion(IData::WRITE);



  delete dr;
  c->clear();
  delete c;
  dt_log.addEventEnd("ReadTask",DuctteipLog::ReadTask);
}
void AddTask ( IContext *ctx,char*s,unsigned long key,IData *d1                    ) { AddTask ( ctx,s,key,NULL,NULL , d1);}
void AddTask ( IContext *ctx,char*s,unsigned long key,IData *d1,IData *d2          ) { AddTask ( ctx,s,key,d1  ,NULL , d2);}

/*===============================================================================*/
void IDuctteipTask::dump(char c){
  if ( !DUMP_FLAG)
    return;
  if(0) printf("#task:%s key:%lx ,no.of data:%ld state:%d expo:%d impo:%d\n ",
	 name.c_str(),key,data_list->size(),state,exported,imported);
  if (type == PropagateTask)
    prop_info->dump();
  else
    dumpDataAccess(data_list);
}
/*--------------------------------------------------------------*/
IDuctteipTask::IDuctteipTask(PropagateInfo *p){
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
/*--------------------------------------------------------------*/
void IDuctteipTask::runPropagateTask(){
  IData *data = glbCtx.getDataByHandle(&prop_info->data_handle);
  data->setRunTimeVersion(prop_info->toCtx,0);
  setFinished(true);
}
/*--------------------------------------------------------------*/
MessageBuffer *IDuctteipTask::serialize(){
  int offset =0 ;
  serialize(message_buffer->address,offset,message_buffer->size);
  return message_buffer;
}
/*--------------------------------------------------------------*/
int IDuctteipTask::serialize(byte *buffer,int &offset,int max_length){
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
  return 0;
}
/*--------------------------------------------------------------*/
void IDuctteipTask::deserialize(byte *buffer,int &offset,int max_length){
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
/*--------------------------------------------------------------*/
void IDuctteipTask::run(){
  if ( state == Finished ) 
    return;
  if ( state == Running ) 
    return;
  if (type == PropagateTask){
    runPropagateTask();
  }
  else{
    state = Running;
    if(0)printf("%s starts running:\n",getName().c_str());
    parent_context->runKernels(this);
  }
}
/*--------------------------------------------------------------*/
void IDuctteipTask::setFinished(bool flag){
  if ( !flag ) 
    return ;
  state = IDuctteipTask::Finished;
  dt_log.addEventEnd(this,DuctteipLog::Executed);
  PRINT_IF(OVERSUBSCRIBED)("-----task:%s finish mutex unlocked,time:%Ld,st:%d,fin=%d,",
			   getName().c_str(),getTime(),state,IDuctteipTask::Finished);
  PRINT_IF(OVERSUBSCRIBED)("remained:%ld\n",dtEngine.getUnfinishedTasks());
}
/*--------------------------------------------------------------*/
void IDuctteipTask::upgradeData(char c){
  list<DataAccess *>::iterator it;
  string s;
  bool dbg=true;
  if(dbg)s=getName();
  if ( type == PropagateTask ) 
    return;
  for ( it = data_list->begin(); it != data_list->end(); ++it ) {
    if(dbg)    s+="\n         - "+(*it)->data->getName()+" "+(*it)->data->dumpVersionString();
    PRINT_IF(DATA_UPGRADE_FLAG)("** data upgraded :%s,%Ld\n",
				(*it)->data->getName().c_str(),
				dtEngine.elapsedTime(TIME_SCALE_TO_MILI_SECONDS));
    (*it)->data->incrementRunTimeVersion((*it)->type);
    if(dbg)    s+="\n                   "+(*it)->data->dumpVersionString();
    dt_log.addEvent((*it)->data, DuctteipLog::RunTimeVersionUpgraded);    

  }
  if(dbg)printf("%c(*)%s %ld\n",c,s.c_str(),getTime());
  state = CanBeCleared;
  dtEngine.putWorkForDataReady(data_list);
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
  data_access->required_version.dump();
  data_request = data_access;
  dump();
}
/*===============================================================================*/
void engine::receivePropagateTask(byte *buffer, int len){
  PropagateInfo *p = new PropagateInfo;
  int offset =0 ;
  p->deserialize(buffer,offset,len);
  addPropagateTask(p);
}
/*--------------------------------------------------------------*/
void engine::addPropagateTask(PropagateInfo *P){
  IDuctteipTask *task = new IDuctteipTask(P);
  string s("PROPTASK");
  task->setName(s);
  criticalSection(Enter) ;
  TaskHandle task_handle = last_task_handle ++;
  task->setHandle(task_handle);
  task_list.push_back(task);
  putWorkForPropagateTask(task);
  criticalSection(Leave) ;
}
void engine:: sendTask(IDuctteipTask* task,int destination){
  MessageBuffer *m = task->serialize();
  unsigned long ch =  mailbox->send(m->address,m->size,MailBox::TaskTag,destination);
  task->setCommHandle (ch);
  dt_log.addEvent(task,DuctteipLog::TaskSent,destination);  
}
/*--------------------------------------------------------------*/
void engine:: exportTask(IDuctteipTask* task,int destination){
  MessageBuffer *m = task->serialize();
  unsigned long ch =  mailbox->send(m->address,m->size,MailBox::MigrateTaskTag,destination);
  task->setCommHandle (ch);
  printf("Task is exported:%s\n",task->getName().c_str());
  task->dump();
  dt_log.addEvent(task,DuctteipLog::TaskExported,destination);  
}
/*=====================================================================*/
void engine::receivedData(MailBoxEvent *event,MemoryItem *mi){
  int offset =0 ;
  const bool header_only = true;
  const bool all_content = false;
  DataHandle dh;
  dh.deserialize(event->buffer,offset,event->length);
  //  flushBuffer(event->buffer,event->length);
  if(0)printf("dh:%ld\n",dh.data_handle);
  IData *data = glbCtx.getDataByHandle(&dh);
  offset =0 ;
  if(0)printf("data rcvd:%s,cnt:%p mi:%p\n",data->getName().c_str(),data->getContentAddress(),event->getMemoryItem());
  data->deserialize(event->buffer,offset,event->length,mi,all_content);
  if(0)printf("data rcvd:%s,cnt:%p, mi:%p\n",data->getName().c_str(),data->getContentAddress(),mi);


  void dumpData(double *, int ,int, char);
  if (0){
    printf("Buffer:%p Header size:%d\n",event->buffer,data->getHeaderSize());
    double *A=(double *)(event->buffer+data->getHeaderSize());
    dumpData(A,6,6,'B');
    double *B=(double *)(data->getContentAddress());
    dumpData(B,6,6,'R');
    printf("MsgBuffer :%p , DataMemory:%p\n",A,B);
  }

  data->dump(' ');
  putWorkForSingleDataReady(data);
  dt_log.addEvent(data,DuctteipLog::DataReceived);
}
/*--------------------------------------------------------------------------*/
IData * engine::importedData(MailBoxEvent *event,MemoryItem *mi){
  int offset =0 ;
  const bool header_only = true;
  const bool all_content = false;
  DataHandle dh;

  /*
  if (0){
    double sum = 0.0,*contents=(double *)(event->buffer+192);
    long size = (event->length -192)/sizeof(double);
    for ( long i=0; i< size; i++)
      sum += contents[i];
    printf("+++sum i , -----,%lf adr:%p,%p,%p\n",
	   sum,contents,event->memory->getAddress(),event->buffer);
  }
  */

  printf ("buf:%p,len:%d\n",event->buffer,event->length);
  dh.deserialize(event->buffer,offset,event->length);
  printf("data handle:%ld\n",dh.data_handle);
  IData *data = glbCtx.getDataByHandle(&dh);
  printf("local data&ver :%s %s\n",data->getName().c_str(),
	 data->dumpVersionString().c_str());
  /*
  void dumpData(double *, int ,int, char);

  if (0){
    double sum = 0.0,*contents=(double *)(event->buffer+192);
    long size = (event->length -192)/sizeof(double);
    for ( long i=0; i< size; i++)
      sum += contents[i];
    printf("+++sum i , -----,%lf adr:%p,%p,%p\n",
	   sum,contents,event->memory->getAddress(),event->buffer);
  }

    if ( 1 ) {
      printf("Buffer:%p Header size:%d\n",event->buffer,data->getHeaderSize());
      double *A=(double *)(event->buffer+data->getHeaderSize());
      dumpData(A,12,12,'I');
    }
    if(0)printf("ev.mem dump:\n   ");
    event->memory->dump();
    if(0)printf("data-before.mem dump:\n   ");
  */

  
    mi = data->getDataMemory();
    if ( mi != NULL ) {      
      memcpy(data->getContentAddress(),event->buffer+data->getHeaderSize(),data->getContentSize());
    }
    else{
      data->setDataMemory(event->memory);
      data->setContentSize(event->length-data->getHeaderSize());
      mi = data->getDataMemory();
      data->prepareMemory();
    }
  
    /*
    if (0 ) {
      MemoryItem *mi2 = data->getDataMemory();
      printf("data-after.mem dump:\n   ");
      mi2->dump();
    }
    
  if ( mi == NULL) {
    printf("preparememory.///////////////////////////////////////////////// \n");
    data->prepareMemory();
  }
  if(0)printf("imported data %s, mem:%p len:%d size:%d sz2:%d\n",data->getName().c_str(),
	 data->getContentAddress(),event->length,
	 data->getContentSize(),event->length-data->getHeaderSize());
    */
  offset=0;
  if(1)printf("data rcvd:%s,cnt:%p mi:%p\n",
	      data->getName().c_str(),
	      data->getContentAddress(),
	      event->getMemoryItem());
  data->deserialize(event->buffer,offset,event->length,mi,header_only);
  printf("imported data&ver :%s %s\n",data->getName().c_str(),
	 data->dumpVersionString().c_str());
  dt_log.addEvent(data,DuctteipLog::DataImported);
  return data;
}
/*--------------------------------------------------------------------------*/
void IListener::checkAndSendData(MailBox * mailbox)
{
  if ( !isReceived() ) {
    return;
  }
  if ( isDataSent() ) 
    {
      return;
    }
  if (! isDataReady() ) {
    IData *data = getData();
    if(0)printf("@lsnr: data  %s is not ready.\n",data->getName().c_str());
    return;
  }
  IData *data = getData();
  if ( data->isDataSent(getSource(), getRequiredVersion() ) ){
    setDataSent(true);
    return;
  }
  data->serialize();
  if(0)printf("@data sent %s  to :%d dh:%ld tag:%d\n",data->getName().c_str(),
	      getSource(),data->getDataHandleID(),MailBox::DataTag);
  unsigned long c_handle= mailbox->send(data->getHeaderAddress(),
					data->getPackSize(),
					MailBox::DataTag, 
					getSource());

  if (0){
    void dumpData(double *, int ,int, char);
    printf("DataMem:%p Header size:%d\n",data->getContentAddress(),data->getHeaderSize());
    double *A=(double *)(data->getContentAddress());
    dumpData(A,6,6,'D');
  }

  dt_log.addEvent(this,DuctteipLog::Woken);
  dt_log.addEvent(data,DuctteipLog::DataSent,getSource());
  setCommHandle(c_handle);
  setDataSent(true);
  if(0)printf("data sent:%s\n",data->getName().c_str());
  dump();
  data->dump(' ');


}
/*--------------------------------------------------------------------------*/
template <class ElementType>
Partition<ElementType> *
Partition<ElementType>::getBlock ( int y, int x ) {

  Partition<ElementType> *p = new Partition<ElementType> (dim_count, element_alignment) ; 
  p->block_alignment = block_alignment ;
  p->mem_size = mem_size;
  p->X_E()  = X_EB();
  p->Y_E()  = Y_EB();

  p->X_B()  = 1; 
  p->Y_B()  = 1; 

  p->Y_BS() = 0; 
  p->X_BS() = 0; 

  p->X_EB() = p->X_E() / p->X_B(); 
  p->Y_EB() = p->Y_E() / p->Y_B(); 

  if ( element_alignment == ROW_MAJOR ) {
    p->X_ES() = 1;
    p->Y_ES() = X_EB();
  }
  else{
    p->X_ES() = Y_EB();
    p->Y_ES() = 1     ;
  }
  if ( block_alignment  == ROW_MAJOR ) {
    p->memory = memory + (y * X_B() + x )* Y_EB() * X_EB() ;
  }
  else { 
    p->memory = memory + (x * Y_B() + y )* Y_EB() * X_EB() ;
  }
  if(0)printf("yeb %d e %d b %d\n",p->Y_EB(),p->Y_E(),p->Y_B());
  return p;
} 
/*----------------------------------------------------------------------------------------*/
void IData:: allocateMemory(){
  if ( Nb == 0 && Mb == 0 ) {      
    
    content_size = (N/parent_data->Nb) * (M/parent_data->Mb) * sizeof(double);
    if (simulation)
      content_size=1;
    data_memory = dtEngine.newDataMemory();

    PRINT_IF(0)("@DataMemory %s block size calc: N:%d,pNb:%d,M:%d,pMb:%d,memory:%p\n",
		getName().c_str(),N,parent_data->Nb,M,parent_data->Mb,getContentAddress() );
  }
}
/*----------------------------------------------------------------------------------------*/
#define DATA_LISTENER 0
bool IData::isDataSent(int _host , DataVersion version){
  list<DataListener *>::iterator it;
  for (it = listeners.begin();it != listeners.end();it ++){
    DataListener *lsnr = (*it);
    if (lsnr->getHost() == _host && lsnr->getRequiredVersion() == version){
      PRINT_IF(DATA_LISTENER)("DLsnr data:%s for host:%d is sent?:%d\n",name.c_str(),_host , lsnr->isDataSent());
      version.dump();
      return  lsnr->isDataSent();
    }
  }
  return false;
}
/*--------------------------------------------------------------------------*/
void IData::dataIsSent(int _host) {
  list<DataListener *>::iterator it;

  PRINT_IF(DATA_LISTENER)("Data:%s,DLsnr sent to host:%d,cur ver:\n",name.c_str(),_host);
  rt_write_version.dump();
  for (it = listeners.begin();it != listeners.end();it ++){
    DataListener *lsnr = (*it);
    if (lsnr->getHost() == _host && lsnr->getRequiredVersion() == rt_write_version){
      PRINT_IF(DATA_LISTENER)("DLsnr rt_read_version before upgrade:\n");
      rt_read_version.dump();
      incrementRunTimeVersion(READ,lsnr->getCount());
      PRINT_IF(DATA_LISTENER)("DLsnr rt_read_version after upgrade:\n");
      rt_read_version.dump();
      lsnr->setDataSent(true);
      //it=listeners.erase(it);
      return;
    }      
  }    
}
/*--------------------------------------------------------------------------*/
void IData::addTask(IDuctteipTask *task){
  //  printf("@@task:%s: added to Data:%s\n",task->getName().c_str(),getName().c_str());
  tasks_list.push_back(task);
}
/*--------------------------------------------------------------------------*/
void IData::listenerAdded(DataListener *exlsnr,int host , DataVersion version ) {
  list<DataListener *>::iterator it;
  version.dump();
  for (it = listeners.begin();it != listeners.end();it ++){
    DataListener *mylsnr = (*it);
    if (mylsnr->getHost() == host && mylsnr->getRequiredVersion() == version){
      mylsnr->setCount(mylsnr->getCount()+1);
      return;
    }
  }
  exlsnr->setCount(1);
  listeners.push_back(exlsnr);
  if(0)printf("data %s, lsnr list size:%ld\n",getName().c_str(),listeners.size());
}
/*--------------------------------------------------------------------------*/
void IData::deleteListenersForOldVersions(){
  list<DataListener *>::iterator it;
  it = listeners.begin();
  for (;it != listeners.end();){
    DataListener *lsnr = (*it);
    if (lsnr->getRequiredVersion() < rt_write_version ){
      PRINT_IF(DATA_LISTENER)("Data:%s,listeners deleted before:\n",name.c_str());
      rt_write_version.dump();
      //it=listeners.erase(it);
    }
    else it ++;
  }
}
/*--------------------------------------------------------------------------*/
void IData::checkAfterUpgrade(list<IDuctteipTask*> &running_tasks,MailBox *mailbox,char debug){
  list<IListener *>::iterator lsnr_it;
  list<IDuctteipTask *>::iterator task_it;

  if(0)printf("data:check after data upgrade lsnr:%ld tasks :%ld\n",listeners.size(),tasks_list.size());
  if (listeners.size() >0) {
    dt_log.addEventStart(this,DuctteipLog::CheckedForListener);
    for(lsnr_it = listeners.begin() ; 
	lsnr_it != listeners.end()  ; 
	++lsnr_it){
      IListener *listener = (*lsnr_it);
      listener->checkAndSendData(mailbox);
    }
    dt_log.addEventEnd(this,DuctteipLog::CheckedForListener);
  }
  if (tasks_list.size() >0) {
    dt_log.addEventStart(this,DuctteipLog::CheckedForTask);
    for(task_it = tasks_list.begin() ; 
	task_it != tasks_list.end()  ;
	++task_it){
      IDuctteipTask *task = (*task_it);
      dt_log.addEventStart(task,DuctteipLog::CheckedForRun);
      /*      if ( task->isExported() ) {
	task->setState(IDuctteipTask::CanBeCleared);
	continue;
	}*/
      if ( !task->isFinished())
	if(0)printf("data %s -> task:%s,stat=%d.\n",getName().c_str(),task->getName().c_str(),task->getState());
	if (task->canRun(debug)) {
	  dt_log.addEventEnd(task,DuctteipLog::CheckedForRun);
	  dt_log.addEventStart(task,DuctteipLog::Executed);
	  running_tasks.push_back(task);
	  if(0)printf("RUNNING Tasks#:%ld\n",running_tasks.size());
	  if(0)printf("task:%s inserted in running q.\n",task->getName().c_str());
	}
	else
	  dt_log.addEventEnd(task,DuctteipLog::CheckedForRun);
    }
    dt_log.addEventEnd(this,DuctteipLog::CheckedForTask);
  }
  dtEngine.runFirstActiveTask();
  
}


#endif //  __CONTEXT_HPP__
