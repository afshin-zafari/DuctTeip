#include "data.hpp"
#include "context.hpp"
#include "glb_context.hpp"


  /*--------------------------------------------------------------------------*/
  int ContextPrefix::serialize(byte *buffer,int &offset,int max_length){
    int len = context_id_list.size();
    copy<int>(buffer,offset,len);
    list<int>::iterator it;
    for(it = context_id_list.begin();it != context_id_list.end();it++){
      int context_id = *it;      
      copy<int>(buffer,offset,context_id);
    }    
    return 0;
  }
  /*--------------------------------------------------------------------------*/
  int ContextPrefix::deserialize(byte *buffer,int &offset,int max_length){
    reset();
    int context_id,count;
    paste<int>(buffer,offset,&count);
    for ( int i =0 ; i< count; i++){
      paste<int>(buffer,offset,&context_id);
      context_id_list.push_back(context_id);
    }
    return 0;
  }
  /*--------------------------------------------------------------------------*/
  bool ContextPrefix:: operator !=(ContextPrefix rhs){
    return !(*this == rhs);
  }
  /*--------------------------------------------------------------------------*/
  bool ContextPrefix::operator ==(ContextPrefix rhs){
    if ( context_id_list.size() == 0 && rhs.context_id_list.size() == 0 ) 
      return true;
    if (context_id_list.size() !=  rhs.context_id_list.size() ) 
      return false;
    list<int>::iterator it1,it2;
    for (it1 = context_id_list.begin(),it2 = rhs.context_id_list.begin();
	 it1 != context_id_list.end() && it2 != rhs.context_id_list.end(); 
	 it1 ++, it2++){
      if (*it1 != *it2) 
	return false;
    }
    return true;
  }
  /*--------------------------------------------------------------------------*/
  void ContextPrefix::fromString(string ctx){
    istringstream s(ctx);
    char c;//1.2.1.
    int context_id;
    reset();
    while ( !s.eof() ) {
      s >> context_id >> c;
      if ( s.eof() ) break;
      context_id_list.push_back(context_id);
    }
  }
  /*--------------------------------------------------------------------------*/
  ContextPrefix ContextPrefix::operator=(ContextPrefix rhs){
    reset();
    //    if(context_id_list.size()>0)
    context_id_list.assign( rhs.context_id_list.begin(),rhs.context_id_list.end());
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  string ContextPrefix::toString(){
    list<int>::iterator it;
    ostringstream s;
    for(it = context_id_list.begin(); it != context_id_list.end(); it ++){      
      s << (*it) << ".";
    }
    return s.str();      
  }
  /*--------------------------------------------------------------------------*/
  void ContextPrefix::dump(){
    if (!DUMP_FLAG)
      return;
    printf("prefix: %s\n",toString().c_str());
  }
  /*--------------------------------------------------------------------------*/
/*==========================DataVersion ======================================*/
  DataVersion DataVersion::operator +=(int a){
    version += a;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion DataVersion::operator =(DataVersion rhs){

    version = rhs.version;
    prefix  = rhs.prefix ;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion DataVersion::operator =(int a){
    version = a;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  bool DataVersion::operator ==(DataVersion rhs){
    if (prefix != rhs.prefix) {
      return false;
    }
    if ( version == rhs.version){
      return true;
    }
    return false;
  }
  /*--------------------------------------------------------------------------*/
  bool DataVersion::operator <(DataVersion rhs){
    if (prefix != rhs.prefix) {
      return false;
    }
    if ( version < rhs.version){
      return true;
    }
    return false;
  }
  /*--------------------------------------------------------------------------*/
  bool DataVersion::operator !=(DataVersion &rhs){
    return !(*this==rhs);
  }
  /*--------------------------------------------------------------------------*/
  DataVersion DataVersion::operator ++(int a) {
    version ++;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion DataVersion::operator ++() {
    version ++;
    return *this;    
  }
  /*--------------------------------------------------------------------------*/
  string DataVersion::dumpString(){
    ostringstream os;
    os << prefix.toString()  << version;
    return os.str();
  }
  /*--------------------------------------------------------------------------*/
  void DataVersion::dump(char c){
    if (!DUMP_FLAG)
      return;
    printf(",version:%s,\n",dumpString().c_str());
  }
  /*--------------------------------------------------------------------------*/
  int  DataVersion::serialize(byte *buffer,int &offset,int max_length){
    copy<int>(buffer,offset,version);
    prefix.serialize(buffer,offset,max_length);
    return 0;
  }
  /*--------------------------------------------------------------------------*/
  int DataVersion::deserialize(byte *buffer,int &offset,int max_length){
    paste<int>(buffer,offset,&version);
    prefix.deserialize(buffer,offset,max_length);
    return 0;
  }
  /*--------------------------------------------------------------------------*/
  void DataVersion::reset(){
    prefix.reset();
    version =0 ;
  }
  /*--------------------------------------------------------------------------*/
/*==========================DataVersion ======================================*/



/*==========================DataHandle =======================================*/

DataHandle &DataHandle::operator = ( DataHandle rhs ) {
    this->context_handle = rhs.context_handle ;
    this->data_handle   = rhs.data_handle    ;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  bool DataHandle::operator == ( DataHandle rhs ) {
    return (this->context_handle == rhs.context_handle &&
	    this->data_handle   == rhs.data_handle    );
  }
  /*--------------------------------------------------------------------------*/
  int    DataHandle::serialize(byte *buffer, int &offset,int max_length){
    copy<unsigned long>(buffer,offset,context_handle);
    copy<unsigned long>(buffer,offset,   data_handle);
    return 0;
  } 
  /*--------------------------------------------------------------------------*/
  void DataHandle::deserialize(byte *buffer, int &offset,int max_length){
    paste<unsigned long>(buffer,offset,&context_handle);
    paste<unsigned long>(buffer,offset,   &data_handle);
  }

/*==========================DataHandle =======================================*/

/*========================== IData Class =====================================*/

  /*--------------------------------------------------------------------------*/
IData::IData(){
    my_data_handle = new DataHandle;
    dtPartition = NULL; 
    hM = NULL;
    name="";
    data_memory=NULL;
  }
  /*--------------------------------------------------------------------------*/
  IData::~IData() {
    if ( my_data_handle) delete my_data_handle;
    my_data_handle = NULL;
    if ( dtPartition !=NULL ) {
      delete dtPartition;
      dtPartition=NULL;
    }
    if ( hM ) {
      for ( int i=0;i<local_mb; i++)
	delete hM[i];
      delete[] hM;
      hM = NULL;
    }

  }

  /*--------------------------------------------------------------------------*/
  byte  *IData::getHeaderAddress(){
    return data_memory->getAddress();
  }
  /*--------------------------------------------------------------------------*/
  double *IData::getContentAddress(){
    return (double *)(data_memory->getAddress() + getHeaderSize());
  }
  /*--------------------------------------------------------------------------*/
  void IData::setDataMemory(MemoryItem *mi ) {
    //    data_memory->setState( MemoryItem::Ready) ; 
    data_memory = mi;
    if(0)printf("======%s\n",getName().c_str());
  }
  /*--------------------------------------------------------------------------*/
  MemoryItem *IData::getDataMemory(){
    return data_memory;
  }
  /*--------------------------------------------------------------------------*/
  void IData::setLocalNumBlocks(int mb, int nb ) {
    local_nb = nb;
    local_mb = mb;
  }
  /*--------------------------------------------------------------------------*/
Handle<Options> **IData::createSuperGlueHandles(){
    if ( hM !=NULL ) 
      return hM;
    //    printf ("SG handle for Data %s is created.\n",getName().c_str());
    int nb=local_nb,mb=local_mb;
    bool dbg=!true;
    hM= new Handle<Options>*[mb];
    for(int i=0;i<mb;i++){
      hM[i]=new Handle<Options>[nb];
    }   
    PRINT_IF(dbg)("data :%s , %p\n memory size:%d, %d,%d\n partition ptr:%p",name.c_str(),getContentAddress(),
		  local_m * local_n,local_m,local_n,dtPartition);
    dtPartition->setBaseMemory( getContentAddress() , local_m * local_n);
    dtPartition->dump();
    dtPartition->partitionRectangle(local_m,local_n,local_mb,local_nb);
    PRINT_IF(dbg)("m:%d,n:%d,mb:%d,nb:%d\n",local_m,local_n,local_mb,local_nb);
    
    for(int i=0;i<mb;i++){
      for(int j=0;j<nb;j++){
	hM[i][j].block =dtPartition->getBlock(i,j);
	PRINT_IF(dbg)("Block(%d,%d).Mem:%p\n",i,j,hM[i][j].block->getBaseMemory());
	PRINT_IF(dbg)("Block(%d,%d).Y_E:%d,X_E:%d\n",i,j,hM[i][j].block->Y_E(),hM[i][j].block->X_E());
	PRINT_IF(dbg)("Block(%d,%d).Y_EB:%d,X_EB:%d\n",i,j,hM[i][j].block->Y_EB(),hM[i][j].block->X_EB());
	sprintf( hM[i][j].name,"M[%d,%d](%d,%d)",blk.by,blk.bx,i,j);
      }
    }
    return hM;
  }
  /*--------------------------------------------------------------------------*/
DataVersion IData::getRunTimeVersion(byte type){ 
    if ( type == IData::WRITE ) 
      return rt_read_version;
    else
      return rt_write_version;

  }
  /*--------------------------------------------------------------------------*/
  void IData::setRunTimeVersion(string to_ctx, int to_version){
    rt_read_version = to_version;
    rt_read_version.setContext(to_ctx);
    rt_write_version = to_version;
    rt_write_version.setContext(to_ctx);
  }
  /*--------------------------------------------------------------------------*/
  void IData::incrementRunTimeVersion(byte type,int v  ){
    exported_nodes.clear();
    if ( type == IData::WRITE ) {
      rt_read_version += v;
      rt_write_version= rt_read_version;
      //deleteListenersForOldVersions();
      dump('W');
    }
    else{
      rt_read_version +=v;
      dump('R');
    }
  }
  /*--------------------------------------------------------------------------*/
  /*--------------------------------------------------------------------------*/
  IData *IData::operator () (const int i,const int j) {    
    return (*dataView)[i][j];  
  }
  /*--------------------------------------------------------------------------*/
  IData *IData::operator [] (const int i) {  return (*dataView)[i][0];    }
  /*--------------------------------------------------------------------------*/
  IData *IData::operator & (void) {        return this;  }
  /*--------------------------------------------------------------------------*/
  double IData::getElement(int row , int col ) {
    double *m = dtPartition->getElementAt(row,col);
    return *m;
  }
  /*--------------------------------------------------------------------------*/  
  void IData::setElement(int row, int col ,double v) {
    double *elem = dtPartition->getElementAt(row,col);
    *elem=v;
  }
  /*--------------------------------------------------------------------------*/  
  void IData::dumpElements(){
    if ( data_memory == NULL)
      return;
    if (local_m> 12) return;
    if (local_n> 12) return;
    printf("Data:%s(%d,%d),adr:%p, hdr-adr:%p\n",
	   getName().c_str(),blk.by,blk.bx,
	   getContentAddress(),
	   getHeaderAddress());
    for ( int i = 0 ; i < local_m; i ++){
      for(int j = 0 ; j < local_n; j ++ ) {
	//printf(" (%d,%d):%3.0lf",i,j,getElement(i,j));
	printf("%3.0lf",getElement(i,j));
      }
      printf("\n");
    }
  }
  /*--------------------------------------------------------------------------*/
  byte *IData::serialize(){
    int offset = 0 ;
    byte * buffer =getHeaderAddress(); 
    serialize(buffer,offset,getHeaderSize() );
    return buffer;
  }
  /*--------------------------------------------------------------------------*/
  void IData::serialize(byte *buffer , int &offset, int max_length){
    my_data_handle->serialize(buffer,offset,max_length);
    gt_read_version.serialize(buffer,offset,max_length);
    gt_write_version.serialize(buffer,offset,max_length);
    rt_read_version.serialize(buffer,offset,max_length);
    rt_write_version.serialize(buffer,offset,max_length);
  }
  /*--------------------------------------------------------------------------*/
  void IData::deserialize(byte *buffer, int &offset,int max_length,MemoryItem *mi,bool header_only ){
    my_data_handle->deserialize(buffer,offset,max_length);
    gt_read_version.deserialize(buffer,offset,max_length);
    gt_write_version.deserialize(buffer,offset,max_length);
    rt_read_version.deserialize(buffer,offset,max_length);
    rt_write_version.deserialize(buffer,offset,max_length);

    if ( !header_only ) {
      if (mi != data_memory){
	printf("!!!!!!!!!!!!\n");
	hM = NULL;
	//createSuperGLueHandles();
      }
      if ( mi != NULL)
	setDataMemory( mi ) ;
      dtPartition->setBaseMemory( getContentAddress(), getContentSize()) ; 
    }
  }
  /*--------------------------------------------------------------------------*/
  int IData::getContentSize(){    return content_size;  }
  /*--------------------------------------------------------------------------*/
  int IData::getHeaderSize(){
    return my_data_handle->getPackSize() + 
      4 * gt_read_version .getPackSize() ;
  }
  /*--------------------------------------------------------------------------*/
  int IData::getPackSize(){ 
    return getHeaderSize() +  getContentSize() ;
  }
  /*--------------------------------------------------------------------------*/
  /*--------------------------------------------------------------------------*/
  /*--------------------------------------------------------------------------*/
  void IData::dump(char ch){
    if ( ch != 'z') return;
    double *contents=getContentAddress();
    int r,c;
    r=c=3;
    for ( int k=0;k<local_mb;k++){
      for (int ii=0;ii<r;ii++){
	for(int l=0;l<local_nb;l++){
	  for (int jj=0;jj<c;jj++){
	    printf(" %3.0lf ",contents[k*r*c+l*local_mb*r*c+ii+jj*r]);
	  }
	}
	printf("\n");
      }
    }
    
  }
  /*--------------------------------------------------------------------------*/
  string  IData::dumpVersionString(){
    string s;
    s = gt_read_version.dumpString() + " " + 
	   gt_write_version.dumpString()+ " " + 
	   rt_read_version.dumpString()+ " " + 
      rt_write_version.dumpString()+ " " ;
    return s;
  }
  /*--------------------------------------------------------------------------*/
  void IData::dumpVersion(){
    if (0 && !DUMP_FLAG)
      return;
    printf("\t\t%s\t\t%s\t\t%s\t\t%s\n",
	   gt_read_version.dumpString().c_str(),
	   gt_write_version.dumpString().c_str(),
	   rt_read_version.dumpString().c_str(),
	   rt_write_version.dumpString().c_str());
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)  {
	(*dataView)[i][j]->dumpVersion();
      }
  }
  /*--------------------------------------------------------------------------*/
  void IData::addToVersion(AccessType axs,int v){
    gt_read_version += v;
    if ( axs == WRITE ) {
      gt_write_version = gt_read_version;
    }
    dump();
  }
  /*--------------------------------------------------------------------------*/
  void IData::resetVersion(){
    gt_read_version = gt_write_version = 0 ;
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)
	(*dataView)[i][j]->resetVersion();
  }

  /*--------------------------------------------------------------------------*/
  DataRange  *IData::RowSlice(int r , int i, int j ) {
    DataRange *dr = new DataRange;
    //dr->d = this;
    dr->row_from = r;
    dr->row_to   = r;
    dr->col_from = i;
    dr->col_to   = j;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  DataRange  *IData::ColSlice(int c , int i, int j ) {
    DataRange *dr = new DataRange;
    //dr->d = this;
    dr->row_from = i;
    dr->row_to   = j;
    dr->col_from = c;
    dr->col_to   = c;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  DataRange  *IData::Region(int fr, int tr, int fc, int tc ) {
    DataRange *dr = new DataRange;
    //dr->d = this;
    dr->row_from = fr;
    dr->row_to   = tr;
    dr->col_from = fc;
    dr->col_to   = tc;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  DataRange  *IData::Cell(int i, int j ) {
    DataRange *dr = new DataRange;
    //dr->d = this;
    dr->row_from = i;
    dr->row_to   = i;
    dr->col_from = j;
    dr->col_to   = j;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  DataRange  *IData::All(){
    DataRange *dr = new DataRange;
    //dr->d = this;
    dr->row_from = 0;
    dr->row_to   = Mb-1;
    dr->col_from = 0;
    dr->col_to   = Nb-1;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  void IData::dumpCheckSum(char c){
	return;
    if (!data_memory)return;
    double *contents=getContentAddress();
    long size = (getContentSize())/sizeof(double);
    double sum = 0.0;
    for ( long i=0; i< size; i++)
      sum += contents[i];
    printf("@CheckSum %c , %s,%lf adr:%p len:%ld CMjr:\n",c,getName().c_str(),sum,contents,size);
    dumpVersion();
  }
  /*--------------------------------------------------------------------------*/
  bool IData::isExportedTo(int p ) {
    list<int>::iterator it;
    it = find(exported_nodes.begin(),exported_nodes.end(),p);
    bool t=  it != exported_nodes.end();
    //printf("Data %s is already sent to %d(y/n)? %c\n",getName().c_str(),p,t?'y':'n');
    return t;
  }
  /*--------------------------------------------------------------------------*/
  void IData::setExportedTo(int p){
    exported_nodes.push_back(p);
  }
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
  printf("Nb:%d,Mb:%d M:%d,N:%d\n",Nb,Mb,M,N);
  int i_ex=0,j_ex=0;
  partial = false;
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
  if ( ctx!=NULL)
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
    if ( parent_data->Nb ==1) {
      c.bx = c.by;
    }
    return ( hpData->getHost ( c,1 )  );
  }
  else{
    return hpData->getHost(blk);
  }
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





/*========================== IData Class =====================================*/


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
  /*
    DataListener *lsnr = new DataListener;
    TRACE_LOCATION;
    DataAccess *dreq = new DataAccess;
    dreq->data = this;
    dreq->required_version = version;
    dreq->type = IData::READ;
    lsnr->setHost(host) ;
    lsnr->setDataRequest(dreq) ;
    TRACE_LOCATION;
    lsnr->setDataSent ( false);
    lsnr->setReceived(true);
  */
  exlsnr->setCount(1);
  listeners.push_back(exlsnr);
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

DuctTeip_Data::DuctTeip_Data(int M, int N):Data("",M,N,NULL){  }
void  DuctTeip_Data::configure(){
  setDataHandle( getParent()->createDataHandle());
  setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
  setLocalNumBlocks(config.nb,config.nb);
  setPartition ( config.Nb,config.Nb ) ;      
}
DuctTeip_Data::DuctTeip_Data(int M, int N,IContext  *alg):Data("",M,N,alg){
  configure();    
}