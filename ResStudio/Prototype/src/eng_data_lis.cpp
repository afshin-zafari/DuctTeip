#include "engine.hpp"
/*--------------------------------------------------------------*/
ulong engine::getDataPackSize(){return dps;}
/*--------------------------------------------------------------*/
void engine::receivedData(MailBoxEvent *event,MemoryItem *mi){
  int offset =0 ;
  //const bool header_only = true;
  const bool all_content = false;
  DataHandle dh;
  dh.deserialize(event->buffer,offset,event->length);

  LOG_INFO(LOG_MULTI_THREAD,"Data handle %ld.\n",dh.data_handle);
  IData *data = glbCtx.getDataByHandle(&dh);
  offset =0 ;
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
  for(it = listener_list.begin(); it != listener_list.end();it ++){
    IListener *lsnr=(*it);
    if (listener->getData()->getDataHandle() == lsnr->getData()->getDataHandle()) {
      if (listener->getRequiredVersion() == lsnr->getRequiredVersion()) {
	return true;
      }
    }
  }
  return false;
}
/*---------------------------------------------------------------------------------*/
bool engine::addListener(IListener *listener ){
  if (isDuplicateListener(listener))
    return false;
  int handle = last_listener_handle ++;
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
  listener->getData()->listenerAdded(listener,host,version);
  criticalSection(Enter);
  listener->setHandle( last_listener_handle ++);
  listener->setCommHandle(-1);
  listener->dump();
  listener_list.push_back(listener);
  putWorkForReceivedListener(listener);
  criticalSection(Leave);

}
/*---------------------------------------------------------------------------------*/
IListener *engine::getListenerForData(IData *data){
  list<IListener *>::iterator it ;
  for(it = listener_list.begin();it != listener_list.end(); it ++){
    IListener *listener = (*it);
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
    if ( listener->getCommHandle() == comm_handle)
      return listener;
  }
  fprintf(stderr,"\nerror:listener not found by comm-handle %ld\n",comm_handle);
  return NULL;
}
/*---------------------------------------------------------------------------------*/
bool engine::isAnyUnfinishedListener(){
  list<IListener *>::iterator it;
  for ( it = listener_list.begin(); it != listener_list.end(); it ++)
    {
      IListener *listener = (*it);
      if ( listener->isReceived() )
	if ( !listener->isDataSent() ) {
	  listener->dump();
	  return true;
	}
    }
  return false;
}
