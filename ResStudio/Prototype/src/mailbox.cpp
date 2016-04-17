
#include "mailbox.hpp"
/*-------------------------------------------------------------------------------*/
MailBoxEvent::MailBoxEvent(byte *b,int l,int t,int s):
  length(l),tag(t),host(s),buffer(b)
{
  memory=NULL;
}
/*-------------------------------------------------------------------------------*/
void MailBoxEvent::setMemoryItem(MemoryItem *m){memory = m ;}
/*-------------------------------------------------------------------------------*/
MemoryItem *MailBoxEvent::getMemoryItem(){return memory;}
/*-------------------------------------------------------------------------------*/
MailBoxEvent::MailBoxEvent(){buffer = NULL; tag = host = length = 0 ;}
/*-------------------------------------------------------------------------------*/
MailBoxEvent::~MailBoxEvent(){ }
/*-------------------------------------------------------------------------------*/
MailBoxEvent &MailBoxEvent::operator =(MailBoxEvent &rhs){
  direction = rhs.direction;
  buffer    = rhs.buffer;
  length    = rhs.length;
  tag       = rhs.tag;
  host      = rhs.host;
  handle    = rhs.handle;
  memory    = rhs.memory;
  return *this;
}
/*-------------------------------------------------------------------------------*/
void MailBoxEvent::dump(){
  if (!DUMP_FLAG)
    return;
  printf("@EVENT:%s, ",direction == Received?"Received":"Sent");
  if (tag ==1 ) printf("Task");
  if (tag ==2 ) printf("Data");
  if (tag ==3 ) printf("Lsnr");
  if (tag ==5 ) printf("TerminateOK");
  printf("\n");
}

/*-------------------------------------------------------------------------------*/
unsigned long  MailBox::send(byte *buffer, int length, int tag, int destination,bool wait){
  LOG_INFO(LOG_MLEVEL,"buf:%p, len:%d, tag:%d\n",buffer,length,tag);
  unsigned long comm_handle  = comm->send(buffer,length,(int)tag,destination,wait);
  return comm_handle ;
}
/*-------------------------------------------------------------------------------*/
inline void MailBox::getLRNeighbors(int org,int *left,int *right){
  int n = comm->get_host_count();
  *right=(org+1) %n;
  if ( (org%2 ==1)){
    *left=org-1;
  }
  else{
    *left = org -1;
    if ( *left < 0 )
      *left +=n;
  }
}

/*-------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------*/
bool MailBox::getEvent(MemoryManager *memman,MailBoxEvent *event,bool *completed,bool wait ){

  int length,source,tag;
  unsigned long handle;
  bool found=false;
#if POST_RECV_DATA == 1
  //LOG_INFO(LOG_MULTI_THREAD,"call anyDataReceived\n");
  assert(comm);
  if ( comm->anyDataReceived(event) ){
    *completed = true;
    return true;
  }
#endif
  found = comm->probe(&tag,&source,&length,wait);
  if (found){
#if POST_RECV_DATA == 1
    if ( tag == DataTag )
      return false;
#endif
    event->direction = MailBoxEvent::Received;
    event->length = length;
    event->host   = source;
    event->tag    = tag ;
    event->handle = 0;
    if ( tag == MigrateDataTag ||
	 tag == DataTag ||
	 tag == MigratedTaskOutDataTag){
      event->memory = memman->getNewMemory();
      event->buffer = event->memory->getAddress();
    }
    else{
      event->memory =  NULL;
      event->buffer = new byte[length];
    }
    int res=comm->receive(event->buffer,length,tag,source,true);
    *completed = (res==0);
    LOG_INFO(LOG_MULTI_THREAD,"buf:%p,result:%d, completed:%d, tag:%d\n",event->buffer,res,*completed,tag);
    return true;
  }

  int dlbtags[7]={
    FindIdleTag,
    FindBusyTag,
    MigrateTaskTag,
    MigrateDataTag,
    DeclineMigrateTag,
    AcceptMigrateTag,
    MigratedTaskOutDataTag
  };
  tag=-1;
  found = comm->probeTags(dlbtags,7,&source,&length,&tag);
  event->tag = -1;
  if(found){
    event->tag = tag;
    event->length = length;
    event->host = source;
    event->direction = MailBoxEvent::Received;
    if (length <=2){
      byte p;
      //int res=
    LOG_INFO(LOG_MLEVEL,"\n");
      comm->receive(&p,1,tag,source,true);
    LOG_INFO(LOG_MLEVEL,"\n");
      *completed = true;
      return true;
    }
    if (  tag == MigrateDataTag || tag == MigratedTaskOutDataTag) {
      event->memory = memman->getNewMemory();
      event->buffer = event->memory->getAddress();
      event->memory->setState(MemoryItem::InUse);
    LOG_INFO(LOG_MULTI_THREAD,"DLB DATA rcv src:%d,tag:%d,len:%d,buf:%p\n",source,tag,length,event->buffer);
    }
    else{
      event->buffer = new byte[event->length];
    }

    //int res=
    comm->receive(event->buffer,length,tag,source,true);
    if(0)printf("et , el, es : %d,%d,%d\n",event->tag,event->length,event->host);
    *completed = true;
    return true;
  }
  if (!found){
    found = comm->isAnySendCompleted(&tag,&handle);
    if ( (tag != TerminateOKTag) && (tag !=  TerminateCancelTag) )
      if ( found ) {
	LOG_INFO(LOG_MULTI_THREAD,"AnySendCompleted=True, tag:%d. \n",tag);
	event->tag = tag;
	event->handle = handle;
	event->direction = MailBoxEvent::Sent;
	*completed =  true;
	return true;
      }
  }
  return false;
}
/*-------------------------------------------------------------------------------*/
void MailBox::waitForAnySendComplete(MailBoxEvent *event){
  int   tag;
  ulong handle;
  comm->waitForAnySendComplete(&tag,&handle);
  if ( (tag != TerminateOKTag) && (tag !=  TerminateCancelTag) ){
    event->tag = tag;
    event->handle = handle;
    event->direction = MailBoxEvent::Sent;
  }
  if ( tag == SelfTerminate )
    self_terminate_sent= true;

}
/*-------------------------------------------------------------------------------*/
void MailBox::waitForAnyReceive(MemoryManager *memman,MailBoxEvent *event){
  int length,source,tag;
  comm->waitForAnyReceive(&tag,&source,&length);
  LOG_INFO(LOG_MULTI_THREAD,"message rcvd src:%d,tag:%d,len:%d\n",source,tag,length);


  event->tag       = tag;
  event->length    = length;
  event->host      = source;
  event->direction = MailBoxEvent::Received;
  if (false && length <=4){
    byte p[4];
    //int res=
    comm->receive(p,length,tag,source,true);
    LOG_INFO(LOG_MULTI_THREAD,"len:%d, tag:%d, src:%d\n",length,tag,source);
    return ;
  }
  if (  tag == DataTag        ||
	tag == MigrateDataTag ||
	tag == MigratedTaskOutDataTag) {
    event->memory = memman->getNewMemory();
    event->buffer = event->memory->getAddress();
    event->memory->setState(MemoryItem::InUse);
  }
  else{
    event->buffer = new byte[event->length];
    LOG_INFO(LOG_MULTI_THREAD,"len:%d, tag:%d, src:%d\n",length,tag,source);
  }

  //int res=
  comm->receive(event->buffer,length,tag,source,true);
  if ( tag == SelfTerminate)
    self_terminate_received=true;
  LOG_INFO(LOG_MULTI_THREAD,"message rcvd src:%d,tag:%d,len:%d,buf:%p\n",source,tag,event->length,event->buffer);
}
/*-------------------------------------------------------------------------------*/
bool MailBox::getSelfTerminate(int send_or_recv){
  if (send_or_recv ==0)
    return self_terminate_sent;
  return  self_terminate_received;

}
/*-------------------------------------------------------------------------------*/
