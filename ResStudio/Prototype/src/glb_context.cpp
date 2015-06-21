#include "glb_context.hpp"
#include "context.hpp"
GlobalContext glbCtx;

  /*-----------------------------------------------------------------------------------------*/
  void PropagateInfo::deserializeContext(byte *buffer,int &offset,int max_length,string &ctx){
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
  void   PropagateInfo::serializeContext(byte *buffer,int &offset,int max_size,string ctx){
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
  int    PropagateInfo::serialize(byte *buffer,int &offset,int max_length){
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
  void PropagateInfo::deserialize(byte *buffer,int &offset,int max_length){
    paste<int>(buffer,offset,&fromVersion);
    paste<int>(buffer,offset,&i);
    paste<int>(buffer,offset,&j);
    paste<unsigned long>(buffer,offset,&data_handle.context_handle);
    paste<unsigned long>(buffer,offset,&data_handle.   data_handle);
    deserializeContext(buffer,offset,max_length,fromCtx);
    deserializeContext(buffer,offset,max_length,  toCtx);
  }



  /*-----------------------------------------------------------------------------------------*/
void ContextHeader::addDataRange(IData::AccessType daxs,DataRange *dr){
    if (daxs == IData::READ)
      readRange.push_back(dr);
    else
      writeRange.push_back(dr);
  }
  /*-----------------------------------------------------------------------------------------*/
  list<DataRange *> ContextHeader::getWriteRange(){
    return writeRange;
  }
  /*-----------------------------------------------------------------------------------------*/
//  list<DataRange *> ContextHeader::getReadRange (){return readRange;}
  /*-----------------------------------------------------------------------------------------*/

  GlobalContext::GlobalContext(){
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
  GlobalContext::~GlobalContext() {
    delete ctxHeader;
  }
  /*-----------------------------------------------------------------------------------------*/
  bool GlobalContext::canAllEnter(){return hpContext->canAllEnter();}
  /*-----------------------------------------------------------------------------------------*/

  ContextHandle *GlobalContext::createContextHandle(){
    ContextHandle *ch = new ContextHandle ; 
    *ch = last_context_handle++;
    return ch;
  }
  /*-----------------------------------------------------------------------------------------*/
  DataHandle  *GlobalContext::createDataHandle (){
    DataHandle *dh = new DataHandle ; 
    dh->data_handle = last_data_handle++ ;
    return dh;
  }

  /*-----------------------------------------------------------------------------------------*/
  void GlobalContext::addContext(IContext *c){
    children.push_back(c);
  }
  /*-----------------------------------------------------------------------------------------*/
  void GlobalContext::getLocalNumBlocks(int *mb, int *nb){
    *nb = cfg->getXLocalBlocks();
    *mb = cfg->getYLocalBlocks();
  }
  /*------------------------------*/
  void GlobalContext::resetCounters() {
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
  void GlobalContext::initPropagateTasks(){
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
  int  GlobalContext::dumpPropagations(list < PropagateInfo *> prop_info){
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
  void GlobalContext::createPropagateTasks(list<DataRange *>  dr){
    list<DataRange *> :: iterator it = dr.begin();
    for ( ; it != dr.end(); ++it ) {
      createPropagateTasksFromRange((*it));
    }
  }
  /*------------------------------*/
  void GlobalContext::createPropagateTasksFromRange(DataRange *  data_range){
  }
  /*------------------------------*/
  void GlobalContext::createPropagateTasks(){
    if ( with_propagation ) {
      if( !getTaskPropagatePolicy()->isAllowed( hpContext,me) )
	return ;
      createPropagateTasks( ctxHeader->getWriteRange() );
      createPropagateTasks( ctxHeader->getReadRange()  );
    } 
  }  
  /*------------------------------*/
  void GlobalContext::resetVersiosOfDataRange(DataRange *  data_range){
  }
  /*------------------------------*/
  void GlobalContext::resetVersiosOfDataRangeList(list<DataRange *>  dr){
    list<DataRange *> :: iterator it = dr.begin();
    for ( ; it != dr.end(); ++it ) {
      resetVersiosOfDataRange((*it));
    }
  }
  /*------------------------------*/
  void GlobalContext::resetVersiosOfHeaderData(){
    resetVersiosOfDataRangeList(ctxHeader->getWriteRange() );
    resetVersiosOfDataRangeList(ctxHeader->getReadRange()  );
  }
  /*------------------------------*/
  void GlobalContext::testPropagatePacking(){
    list < PropagateInfo *> prop_info;
    DataHandle dh;// (10,25);

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
  void GlobalContext::unpackPropagateTask(byte *buffer,int length){
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
  void GlobalContext::packPropagateTask(list < PropagateInfo *> prop_info,int dest_host){
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
  void GlobalContext::sendPropagateTasks(){
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
  void GlobalContext::downLevel(){
    LevelInfo li  = lstLevels.back();
    li.seq = li.children;
    li.children = 0;
    lstLevels.push_back(li);
  }
  /*------------------------------*/
  void GlobalContext::upLevel(){
    LevelInfo  li = lstLevels.back();
    lstLevels.pop_back();
    LevelInfo  liP = lstLevels.back();
    lstLevels.pop_back();
    liP.children ++;
    lstLevels.push_back(liP);
  }
  /*------------------------------*/
  int  GlobalContext::getLevelID(int level){
    list <LevelInfo>::iterator it=lstLevels.begin();
    for ( it  = lstLevels.begin();
	  it != lstLevels.end(),level>0; ++it,--level ) ;
    return (*it).seq ;
  }
  /*------------------------------*/
  string GlobalContext::getLevelString(){
    list <LevelInfo>::iterator it;
    ostringstream _s;
    for ( it = lstLevels.begin(); it != lstLevels.end(); ++it ) {
      _s << (*it).seq <<".";
    }
    return _s.str();
  }
  /*------------------------------*/
  void GlobalContext::dumpLevels(char c){
    printf("%c:\n%s\n",c,getLevelString().c_str());
  }
  /*------------------------------*/
  bool GlobalContext::isAnyOwnedBy(ContextHeader* hdr,int me){
     if ( isAnyOwnedBy(hdr->getWriteRange(),me))
       return true;
     return isAnyOwnedBy(hdr->getReadRange(),me);
   }
  /*------------------------------*/
  bool GlobalContext::isAnyOwnedBy(list<DataRange*> dr,int me){
    return false;
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
