#include "engine.hpp"
/*--------------------------------------------------------------*/
ulong engine::getDataPackSize(){return dps;}
/*--------------------------------------------------------------*/
void engine::receivedData(MailBoxEvent *event,MemoryItem *mi){
  int offset =0 ;
  //const bool header_only = true;
  const bool all_content = false;
  DataHandle dh;
  assert(event);
  dh.deserialize(event->buffer,offset,event->length);

  LOG_INFO(LOG_MULTI_THREAD,"Data handle %ld.\n",dh.data_handle);
  IData *data = glbCtx.getDataByHandle(&dh);
  assert(data);
  offset =0 ;
  #if UAMD_COMM
  if (event->tag == MailBox::UAMDataTag){
    data->setContentSize (event->length- data->getHeaderSize());
    data->setNewMemoryInfo(mi);
    data->deserialize(event->buffer,offset,event->length,mi,all_content);
  }
  else
  #endif
    data->deserialize(event->buffer,offset,event->length,NULL,all_content);
  //  if ( mi != NULL )    mi->setState(MemoryItem::Ready);
  LOG_INFO(LOG_MULTI_THREAD+LOG_DATA,"data:%s, cntnt:%p, ev.buf:%p\n",
	   data->getName().c_str(),data->getContentAddress(),event->buffer);

  dtEngine.putWorkForSingleDataReady(data);
  LOG_EVENT(DuctteipLog::DataReceived);
}
/*---------------------------------------------------------------------------------*/
bool engine::isDuplicateListener(IListener * listener){
  list<IListener *>::iterator it;
  assert(listener);
  LOG_INFO(LOG_LISTENERS,"\n");
  for(it = listener_list.begin(); it != listener_list.end();it ++){
    IListener *lsnr=(*it);
    assert(lsnr);
    if ( lsnr->getData() ){
      if (listener->getData()->getDataHandle() == lsnr->getData()->getDataHandle()) {
	if (listener->getRequiredVersion() == lsnr->getRequiredVersion()) {
	  return true;
	}
      }
    }
    else{
      LOG_INFO(LOG_LISTENERS,"Listener has been added by null data.\n");
    }
  }
  return false;
}
/*---------------------------------------------------------------------------------*/
bool engine::addListener(IListener *listener ){
  assert(listener);
  if (isDuplicateListener(listener)){
    /*
    LOG_INFO(LOG_LISTENERS,"Listenr duplicate for %s, from node:%d, version:%s\n",
	     listener->getData()->getName().c_str(),
	     listener->getHost(),
	     listener->getRequiredVersion().dumpString().c_str());
    listener->getData()->listenerAdded(listener,listener->getHost(), listener->getRequiredVersion());
    */
    return false;
  }
  int handle = last_listener_handle ++;
  if ( listener->getData() ){
    LOG_INFO(LOG_LISTENERS,"Listener added for data :%s\n",listener->getData()->getName().c_str());
  }
  else {
    LOG_INFO(LOG_LISTENERS,"Listener added for data :%p\n",listener->getData() );
  }
  listener_list.push_back(listener);
  listener->setHandle(handle);
  listener->setCommHandle(-1);
  LOG_EVENT(DuctteipLog::ListenerDefined);

  return true;
}
/*---------------------------------------------------------------------------------*/
void engine::receivedListener(MailBoxEvent *event){
  IListener *listener = new IListener;
  int offset = 0 ;
  assert(event);
  assert(listener);
  LOG_EVENT(DuctteipLog::ListenerReceived);

  LOG_INFO(LOG_LISTENERS,"ev.buf:%p ev.len:%d\n",event->buffer,event->length);
  listener->deserialize(event->buffer,offset,event->length);
  listener->setHost ( me ) ;
  listener->setSource ( event->host ) ;
  listener->setReceived(true);
  listener->setDataSent(false);

  IData *data= listener->getData();
  if ( data ==NULL){
    LOG_INFO(LOG_MULTI_THREAD,"Invalid data in listener.\n");
    return;
  }
  int host = event->host;
  DataVersion version = listener->getRequiredVersion();
  LOG_INFO(LOG_LISTENERS,"(****)Ask the data to add it if not duplicate based on host:%d, ver:%s.\n",host,version.dumpString().c_str());
  listener = data->listenerAdded(listener,host,version);


  listener->checkAndSendData(mailbox);

}
/*---------------------------------------------------------------------------------*/
void engine::receivedListener_old(MailBoxEvent *event){
  IListener *listener = new IListener;
  int offset = 0 ;

  LOG_EVENT(DuctteipLog::ListenerReceived);

  LOG_INFO(LOG_LISTENERS,"ev.buf:%p ev.len:%d\n",event->buffer,event->length);
  listener->deserialize(event->buffer,offset,event->length);
  listener->setHost ( me ) ;
  listener->setSource ( event->host ) ;
  listener->setReceived(true);
  listener->setDataSent(false);

  int host = event->host;
  DataVersion version = listener->getRequiredVersion();
  version.dump();
  IData *data= listener->getData();
  if ( data ==NULL){
    LOG_INFO(LOG_MULTI_THREAD,"invalid data \n");
    return;
  }
  LOG_INFO(LOG_LISTENERS,"(****)Listener received from :%d, for data %s,ver:%s.\n",event->host,data->getName().c_str(), version.dumpString().c_str());
  if (data->isDataSent(event->host,version) ){
    listener->setDataSent(true);
    data->incrementRunTimeVersion(IData::READ);
    LOG_INFO(LOG_LISTENERS,"(****)The data is already sent. ver upgraded to %s, putWorkFSDdata\n",data->getRunTimeVersion(IData::READ).dumpString().c_str());
    putWorkForSingleDataReady(data);
    return;
  }
  LOG_INFO(LOG_LISTENERS,"(****)Ask the data to add it if not duplicate based on host:%d, ver:%s.\n",host,version.dumpString().c_str());
  listener->getData()->listenerAdded(listener,host,version);
  criticalSection(Enter);
  listener->setHandle( last_listener_handle ++);
  listener->setCommHandle(-1);
  listener->dump();
  //listener_list.push_back(listener);
  LOG_INFO(LOG_LISTENERS,"(****)Pusshed to listener list of dtEngine.\n");
  putWorkForReceivedListener(listener);
  criticalSection(Leave);

}
/*---------------------------------------------------------------------------------*/
IListener *engine::getListenerForData(IData *data){
  list<IListener *>::iterator it ;
  assert(data);
  for(it = listener_list.begin();it != listener_list.end(); it ++){
    IListener *listener = (*it);
    assert(listener);
    if (listener->getData()->getDataHandle() == data->getDataHandle() )
      if ( listener->getRequiredVersion() == data->getRunTimeVersion(IData::READ) )
	{
	  return listener;
	}
  }
  return NULL;
}
/*---------------------------------------------------------------------------------*/
void engine::dumpListeners(){
  list<IListener *>::iterator it;
  for (it = listener_list.begin(); it != listener_list.end(); it ++){
    (*it)->dump();
  }
}
/*---------------------------------------------------------------------------------*/
void engine::removeListenerByHandle(int handle ) {
  list<IListener *>::iterator it;
  for ( it = listener_list.begin(); it != listener_list.end(); it ++){
    IListener *listener = (*it);
    if (listener->getHandle() == handle){
      criticalSection(Enter);
      LOG_INFO(LOG_LISTENERS,"Listener for data %s removed.\n",listener->getData()->getName().c_str());
      listener_list.erase(it);
      criticalSection(Leave);
      return;
    }
  }
}
/*---------------------------------------------------------------------------------*/
IListener *engine::getListenerByCommHandle ( unsigned long  comm_handle ) {
  list<IListener *>::iterator it;
  for (it = listener_list.begin(); it !=listener_list.end();it ++){
    IListener *listener=(*it);
    assert(listener);
    if ( listener->getCommHandle() == comm_handle)
      return listener;
    else{
      LOG_INFO(LOG_LISTENERS,"lsnr h:%ld, ch:%ld.\n",listener->getCommHandle(),comm_handle);
    }
  }
  //  fprintf(stderr,"\nerror:listener not found by comm-handle %ld\n",comm_handle);
  return NULL;
}
/*---------------------------------------------------------------------------------*/
bool engine::isAnyUnfinishedListener(){
  list<IListener *>::iterator it;
  for ( it = listener_list.begin(); it != listener_list.end(); it ++)
    {
      IListener *listener = (*it);
      assert(listener);
      if ( listener->isReceived() )
	if ( !listener->isDataSent() ) {
	  listener->dump();
	  return true;
	}
    }
  return false;
}
