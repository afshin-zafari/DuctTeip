#include "context.hpp"
/*===============================  Context Class =======================================*/
/*--------------------------------------------------------------------------------------*/
IContext::IContext(string _name){
    name=_name;
    parent =NULL;
    setContextHandle ( glbCtx.createContextHandle() ) ;
    glbCtx.addContext( this ) ;
  }
/*--------------------------------------------------------------------------------------*/
  IContext::~IContext(){
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      delete (*ctx_it);
    }
    inputData.clear();
    outputData.clear();
    children.clear();
  }
/*--------------------------------------------------------------------------------------*/
  IData *IContext::getDataFromList(list<IData* > dlist,int index){
    list<IData *>:: iterator it;
    if ( index >= dlist.size() ) 
      return NULL;
    it=dlist.begin();
    advance(it,index);
    return (*it);
  }
/*--------------------------------------------------------------------------------------*/
  IData *IContext::getOutputData  (int index){    
    return getDataFromList ( outputData , index);
  }
/*--------------------------------------------------------------------------------------*/
  IData *IContext::getInputData   (int index){    
    return getDataFromList ( inputData , index);
  }
/*--------------------------------------------------------------------------------------*/
  IData *IContext::getInOutData   (int index){    
    return getDataFromList ( in_outData , index);
  }
/*--------------------------------------------------------------------------------------*/
  IData *IContext::getDataByHandle(list<IData *> dlist,DataHandle *dh){
    list<IData*> :: iterator it;
    IData* data;
    for (it = dlist.begin(); it != dlist.end();++it) {
      data = (*it)->getDataByHandle(dh);
      if ( data ) {
	if ( data ->getName().size() != 0 ) 
	  return data;
      }
    }
    return NULL;
  }
/*--------------------------------------------------------------------------------------*/
  IData *IContext::getDataByHandle(DataHandle *dh ) {
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

/*--------------------------------------------------------------------------------------*/
  void IContext::setParent ( IContext *_p ) {
    parent = _p;
    parent->children.push_back(this);
  }
/*--------------------------------------------------------------------------------------*/
  void IContext::traverse(){
    if ( parent != NULL )
      parent->print_name("-");
    else
      cout << "-" << endl;
    print_name("--");
    for (list<IContext *>::iterator it=children.begin();it !=children.end();it++)
      (*it)->print_name("---");
  }
/*--------------------------------------------------------------------------------------*/
  void IContext::addInputData(IData *_d){
    inputData.push_back(_d);
  }
/*--------------------------------------------------------------------------------------*/
  void IContext::addOutputData(IData *_d){
    outputData.push_back(_d);
  }
/*--------------------------------------------------------------------------------------*/
  void IContext::addInOutData(IData *_d){
    in_outData.push_back(_d);
    LOG_INFO(LOG_MULTI_THREAD,"add in-out data ptr:%p, len:%d\n",_d,in_outData.size());
  }
/*--------------------------------------------------------------------------------------*/
  void IContext::resetVersions(){
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
/*--------------------------------------------------------------------------------------*/
  void IContext::dumpDataVersions(ContextHeader *hdr){
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
/*--------------------------------------------------------------------------------------*/
  void IContext::testHandles(list<IData*> dlist){
    list<IData*>::iterator it ;
    for (it = dlist.begin(); it != dlist.end(); ++it) {
      (*it)->testHandles();
    }
  }
/*--------------------------------------------------------------------------------------*/
  void IContext::testHandles(){
    IContext * c = glbCtx.getContextByHandle(*my_context_handle);
    printf("Context:%s - Handle:%ld <--> %s,%ld\n",name.c_str(),*my_context_handle,c->getName().c_str(),*c->getContextHandle());
    printf("InputData Handles \n");
    testHandles( inputData);
    printf("Output Data Handles \n");
    testHandles(outputData);
    printf("In/Out Data  Handles \n");
    testHandles(in_outData);
  }

/*--------------------------------------------------------------------------------------*/
  DataHandle * IContext::createDataHandle ( ) {
    DataHandle *d =glbCtx.createDataHandle () ;
    d->context_handle = *my_context_handle ; 
    if(0)printf("@data se dh:%ld\n",d->data_handle);
    return d;
  }
 

/*===================================================================================*/

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
	       string s,
	       unsigned long key,
	       IData *d1,
	       IData *d2,
	       IData *d3){
  ContextHeader *c=NULL;
  if ( !glbCtx.getTaskReadHostPolicy()->isAllowed(ctx,c) ){
    printf("ctx read task disallowed.\n");
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
  LOG_EVENT(DuctteipLog::ReadTask);

  

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
    
    if (d1) d1->dump(' ');
    if (d2) d2->dump(' ');
    d3->dump(' ');
    
  }

  if ( d1 != NULL ) d1->incrementVersion(IData::READ);
  if ( d2 != NULL ) d2->incrementVersion(IData::READ);
  d3->incrementVersion(IData::WRITE);

  delete dr;
  c->clear();
  delete c;
}
void AddTask ( IContext *ctx,string s,unsigned long key,IData *d1                    ) { AddTask ( ctx,s,key,NULL,NULL , d1);}
void AddTask ( IContext *ctx,string s,unsigned long key,IData *d1,IData *d2          ) { AddTask ( ctx,s,key,d1  ,NULL , d2);}
/*===============================================================================*/
