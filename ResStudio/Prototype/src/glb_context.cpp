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
    last_context_handle = 0 ;
    last_data_handle = 0 ;
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
  void GlobalContext::downLevel(){
    LevelInfo li  = lstLevels.back();
    li.seq = li.children;
    li.children = 0;
    lstLevels.push_back(li);
  }
  /*------------------------------*/
  void GlobalContext::upLevel(){
    //LevelInfo  li = lstLevels.back();
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
  //IData * data=NULL ;
  /*
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
  */
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
/*===================================================================================*/
void PropagateInfo::dump(){
  IData *data  = glbCtx.getDataByHandle(&data_handle);
  printf("prop: %s [%s%d] -> [%s0]\n",
	 data->getName().c_str(),fromCtx.c_str(),fromVersion,toCtx.c_str());
}
