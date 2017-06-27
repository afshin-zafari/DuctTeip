#include "listener.hpp"
#include "mailbox.hpp"
#include "glb_context.hpp"



/*--------------------------------------------------------------------------*/
IListener::IListener(){
  message_buffer = NULL;
  data_request=NULL;
}
/*--------------------------------------------------------------------------*/
IListener::IListener(DataAccess* _d , int _host):data_request(_d) , host(_host){
  comm_handle = -1;
  received=false;
  int size = data_request->getPackSize()+ sizeof(host);
  message_buffer = new MessageBuffer(size,0);
  count = 1;
  source = -1;  
}
/*--------------------------------------------------------------------------*/
IListener::~IListener(){
  if  ( message_buffer != NULL )
    delete message_buffer;
}

/*--------------------------------------------------------------------------*/
int           IListener::getHandle     ()                 { return handle;      }
void          IListener::setHandle     (int h )           { handle = h ;        }
unsigned long IListener::getCommHandle ()                 { return comm_handle; }
void          IListener::setCommHandle (unsigned long ch) { comm_handle = ch ;  }
void          IListener::setHost       (int h )           { host = h ;          }
int           IListener::getHost       ()                 { return host;        }
int           IListener::getCount()  { return count;}
void          IListener::setCount(int c ) { count = c; }
IData *IListener::getData() {
  if ( data_request)
    return data_request->data;
  return nullptr;
    
}
/*--------------------------------------------------------------------------*/
DataVersion & IListener::getRequiredVersion(){
  assert(data_request);
  return data_request->required_version;
}
/*--------------------------------------------------------------------------*/
void IListener::setDataRequest ( DataAccess *dr){
  if( dr == nullptr )
    LOG_INFO(LOG_LISTENERS,"Null data request is set for listener.\n");
  data_request = dr;
}
/*--------------------------------------------------------------------------*/
void IListener::setRequiredVersion(DataVersion rhs){
  assert(data_request);
  data_request->required_version = rhs;
}
/*--------------------------------------------------------------------------*/
void IListener::dump(){
  if (!DUMP_FLAG)
    return;
  assert(data_request);
  data_request->required_version.dump();
}
/*--------------------------------------------------------------------------*/
void IListener::setSource(int s ) { source =s ; }
int IListener::getSource(){return source;}
void IListener::setDataSent(bool sent){ data_sent = sent ;}
bool IListener::isDataRemote(){return (!data_request->data->isOwnedBy(me)  );}
bool IListener::isDataSent(){ return data_sent;  }
void IListener::setReceived(bool r) { received = r;}
bool IListener::isReceived() { return received;}
/*--------------------------------------------------------------------------*/
bool IListener::isDataReady(){
  assert(data_request);
  LOG_INFO(LOG_LISTENERS,"dh:%d rt-ver:%s, rq-ver:%s \n",
	   data_request->data->getDataHandleID(),
	   data_request->data->getRunTimeVersion(IData::READ).dumpString().c_str(),
	   data_request->required_version.dumpString().c_str());
  
  bool isReady = (data_request->data->getRunTimeVersion(IData::READ) == data_request->required_version);
  data_request->data->dump();
  data_request->data->getRunTimeVersion(IData::READ).dump();
  data_request->required_version.dump();
  return isReady;
}
/*--------------------------------------------------------------------------*/
long IListener::getPackSize(){
  DataHandle dh;
  DataVersion dv;
    
  return sizeof(host) + dh.getPackSize() + dv.getPackSize();
}
/*--------------------------------------------------------------------------*/
MessageBuffer *IListener::serialize(){
  int offset =0 ;
  serialize(message_buffer->address,offset,message_buffer->size);
  return message_buffer;
}
/*--------------------------------------------------------------------------*/
void IListener::serialize(byte *buffer, int &offset, int max_length){
  assert(data_request);
  copy<int>(buffer,offset,host);
  data_request->data->getDataHandle()->serialize(buffer,offset,max_length);
  data_request->required_version.serialize(buffer,offset,max_length);
} 
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
void IListener::checkAndSendData(MailBox * mailbox)
{
  if ( !isReceived() ) {
    LOG_INFO(LOG_LISTENERS,"Listener is not received.\n");
    return;
  }
  if ( isDataSent() )  {
    LOG_INFO(LOG_LISTENERS,"Listener data is already sent.\n");
    return;
    }
  if (! isDataReady() ) {
    LOG_INFO(LOG_LISTENERS,"Listener data is not ready yet.\n");
    return;
  }
  IData *data = getData();
  if ( data->isDataSent(getSource(), getRequiredVersion() ) ){
    setDataSent(true);
    return;
  }
  data->serialize();
  data->dumpElements();
  unsigned long c_handle=
    mailbox->send(data->getHeaderAddress(),
		  data->getPackSize(),
		  #ifdef UAMD_COMM
		  data->getMemoryType() == IData::USER_ALLOCATED?MailBox::UAMDataTag: MailBox::DataTag,
		  #else
		  MailBox::DataTag,
		  #endif
		  getSource());

  LOG_INFO(LOG_LISTENERS,"data sent %s dh:%ld, comm-handle:%d,count:%d\n",data->getName().c_str(),data->getDataHandleID(),c_handle,getCount());
  data->dumpElements();
  setCommHandle(c_handle);
  setDataSent(true);

  LOG_METRIC(DuctteipLog::DataSent);


}
/*--------------------------------------------------------------------------*/
void IListener::deserialize(byte *buffer, int &offset, int max_length){
  DataAccess *data_access = new DataAccess;
  DataHandle *data_handle = new DataHandle;
  paste<int>(buffer,offset,&host);
  data_handle->deserialize(buffer,offset,max_length);
  IData *data = glbCtx.getDataByHandle(data_handle);
  LOG_INFO(LOG_LISTENERS,"dh:%ld datap:%p\n",data_handle->data_handle,data);
  data_access->required_version.deserialize(buffer,offset,max_length);
  data_access->data = data;
  data_access->required_version.dump();
  data_request = data_access;
  assert(data_request);
  LOG_INFO(LOG_LISTENERS,"Data for lsnr:%s\n",data_request->data->getName().c_str());
}
/*===============================================================================*/
