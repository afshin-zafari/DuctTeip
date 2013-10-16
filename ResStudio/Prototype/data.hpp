#ifndef __DATA_HPP__
#define __DATA_HPP__

#include <string>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <new>
#include "basic.hpp"
#include "partition.hpp"
#include "memory_manager.hpp"

using namespace std;
extern int me;
class IData;
class IHostPolicy ;

/*===================ContextPrefix=======================================================*/
struct  ContextPrefix{
  list<int> context_id_list;
  ContextPrefix(){  }
  ContextPrefix(string s){    fromString(s);  }
  ~ContextPrefix(){    reset();  }
  /*--------------------------------------------------------------------------*/
  int getPackSize(){    return 10* sizeof(int);  }
  /*--------------------------------------------------------------------------*/
  int   serialize(byte *buffer,int &offset,int max_length){
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
  int deserialize(byte *buffer,int &offset,int max_length){
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
  bool operator !=(ContextPrefix rhs){
    return !(*this == rhs);
  }
  /*--------------------------------------------------------------------------*/
  bool operator ==(ContextPrefix rhs){
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
  void fromString(string ctx){
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
  ContextPrefix operator=(ContextPrefix rhs){
    reset();
    //    if(context_id_list.size()>0)
    context_id_list.assign( rhs.context_id_list.begin(),rhs.context_id_list.end());
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  string toString(){
    list<int>::iterator it;
    ostringstream s;
    TRACE_LOCATION;
    for(it = context_id_list.begin(); it != context_id_list.end(); it ++){      
      s << (*it) << ".";
    }
    return s.str();      
  }
  /*--------------------------------------------------------------------------*/
  void dump(){
    if (!DUMP_FLAG)
      return;
    printf("prefix: %s\n",toString().c_str());
  }
  /*--------------------------------------------------------------------------*/
  void reset(){
    context_id_list.clear();
  }
};
/*==========================ContextPrefix=======================================*/

/*==========================DataVersion ========================================*/
struct DataVersion{
public:  
  ContextPrefix prefix;
  int version;
  /*--------------------------------------------------------------------------*/
  int getVersion(){
    return version;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion(){
    version =0 ;
  }
  ~ DataVersion(){
  }
  /*--------------------------------------------------------------------------*/
  int getPackSize(){
    return prefix.getPackSize() + sizeof(version);
  }
  /*--------------------------------------------------------------------------*/
  DataVersion operator +=(int a){
    version += a;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion operator =(DataVersion rhs){
    version = rhs.version;
    prefix  = rhs.prefix ;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion operator =(int a){
    version = a;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  bool operator ==(DataVersion rhs){
    if (prefix != rhs.prefix) {
      return false;
    }
    if ( version == rhs.version){
      return true;
    }
    return false;
  }

  /*--------------------------------------------------------------------------*/
  bool operator <(DataVersion rhs){
    if (prefix != rhs.prefix) {
      return false;
    }
    if ( version < rhs.version){
      return true;
    }
    return false;
  }
  /*--------------------------------------------------------------------------*/
  bool operator !=(DataVersion &rhs){
    return !(*this==rhs);
  }
  /*--------------------------------------------------------------------------*/
  DataVersion operator ++(int a) {
    version ++;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  DataVersion operator ++() {
    version ++;
    return *this;    
  }
  /*--------------------------------------------------------------------------*/
  string dumpString(){
    ostringstream os;
    TRACE_LOCATION;
    os << prefix.toString()  << version;
    return os.str();
  }
  /*--------------------------------------------------------------------------*/
  void dump(){
    if (!DUMP_FLAG)
      return;
    printf(",version:%s,\n",dumpString().c_str());
  }
  /*--------------------------------------------------------------------------*/
  int   serialize(byte *buffer,int &offset,int max_length){
    copy<int>(buffer,offset,version);
    prefix.serialize(buffer,offset,max_length);
  }
  /*--------------------------------------------------------------------------*/
  int deserialize(byte *buffer,int &offset,int max_length){
    paste<int>(buffer,offset,&version);
    prefix.deserialize(buffer,offset,max_length);
  }
  /*--------------------------------------------------------------------------*/
  void reset(){
    prefix.reset();
    version =0 ;
  }
  /*--------------------------------------------------------------------------*/
  void setContext(string s ){
    prefix.fromString(s);
  }
};
/*==========================DataVersion ========================================*/

/*==============================================================================*/
class Coordinate
{
public :
  int by,bx;
  Coordinate(int i , int j ) : by(i),bx(j){}
  Coordinate( ) : by(0),bx(0){}
};//Coordinate

/*==============================================================================*/

typedef struct {
  IData *d;
  int row_from,row_to,col_from,col_to;
}DataRange;


/*==========================DataHandle =========================================*/
struct DataHandle{
public:
  unsigned long int context_handle,data_handle;
  DataHandle () {context_handle=data_handle=0;}
  DataHandle (unsigned long int c,unsigned long int d ){
    context_handle = c;
    data_handle = d;
  }
  /*--------------------------------------------------------------------------*/
  int getPackSize(){
    return sizeof ( context_handle ) + sizeof(data_handle);
  }
  /*--------------------------------------------------------------------------*/
  
  DataHandle operator = ( DataHandle rhs ) {
    this->context_handle = rhs.context_handle ;
    this->data_handle   = rhs.data_handle    ;
  }
  /*--------------------------------------------------------------------------*/
  bool operator == ( DataHandle rhs ) {
    return (this->context_handle == rhs.context_handle &&
	    this->data_handle   == rhs.data_handle    );
  }
  /*--------------------------------------------------------------------------*/
  int    serialize(byte *buffer, int &offset,int max_length){
    copy<unsigned long>(buffer,offset,context_handle);
    copy<unsigned long>(buffer,offset,   data_handle);
  } 
  /*--------------------------------------------------------------------------*/
  void deserialize(byte *buffer, int &offset,int max_length){
    paste<unsigned long>(buffer,offset,&context_handle);
    paste<unsigned long>(buffer,offset,   &data_handle);
  }
};
/*==========================DataHandle =========================================*/
/*==========================DataListener =========================================*/
struct DataListener{
  DataVersion version;
  int host,count;
  bool sent;
};
/*==========================DataListener =====================================*/

class IContext;


/*========================== IData Class =====================================*/
class IData
{
protected:
  string                      name;
  int                         N,M,Nb,Mb;
  Coordinate                  blk;
  vector< vector<IData*> >   *dataView;
  DataVersion                 gt_read_version,gt_write_version;
  DataVersion                 rt_read_version,rt_write_version;
  IContext                   *parent_context;
  IData                      *parent_data;
  IHostPolicy                *hpData;
  DataHandle                 *my_data_handle;
  MemoryItem                 *data_memory;
  list<DataListener *>       listeners;
  int                        local_n, local_m,local_nb,local_mb,content_size;
  Partition<double>          *dtPartition;
  Handle<Options> **hM;
public:
  /*--------------------------------------------------------------------------*/
  enum AccessType {    READ  = 1,    WRITE = 2  };
  /*--------------------------------------------------------------------------*/
  IData(){
    my_data_handle = new DataHandle;
    dtPartition = NULL; 
    hM = NULL;
    name="";
  }
  /*--------------------------------------------------------------------------*/
   IData(string _name,int m, int n,IContext *ctx);
  /*--------------------------------------------------------------------------*/
  ~IData() {
    if ( my_data_handle) delete my_data_handle;
    if ( dtPartition ) delete dtPartition;
    if ( hM ) {
      for ( int i=0;i<local_mb; i++)
	delete hM[i];
      delete[] hM;
    }
  }

  /*--------------------------------------------------------------------------*/
  string        getName          ()                { return name;}
  IContext     *getParent        ()                { return parent_context;}
  IData        *getParentData    ()                { return parent_data;}
  void          setParent        (IContext *p)     { parent_context=p;}
  DataVersion & getWriteVersion()                { return gt_write_version;}
  DataVersion & getReadVersion()                { return gt_read_version;}
  IHostPolicy  *getDataHostPolicy()                { return hpData;}
  void          setDataHostPolicy(IHostPolicy *hp) { hpData=hp;}
  void          setDataHandle    ( DataHandle *d)  { my_data_handle = d;}
  DataHandle   *getDataHandle    ()                { return my_data_handle ; }
  
  /*--------------------------------------------------------------------------*/
  void allocateMemory();
  /*--------------------------------------------------------------------------*/
  byte  *getHeaderAddress(){
    return data_memory->getAddress();
  }
  /*--------------------------------------------------------------------------*/
  double *getContentAddress(){
    return (double *)(data_memory->getAddress() + getHeaderSize());
  }
  /*--------------------------------------------------------------------------*/
  void setDataMemory(MemoryItem *mi ) {
    data_memory->setState( MemoryItem::Ready) ; 
    data_memory = mi;
  }
  /*--------------------------------------------------------------------------*/
  void setLocalNumBlocks(int mb, int nb ) {
    local_nb = nb;
    local_mb = mb;
  }
  /*--------------------------------------------------------------------------*/
  int  getXLocalNumBlocks( ) {    return local_nb;  }
  /*--------------------------------------------------------------------------*/
  int  getYLocalNumBlocks( ) {    return local_mb;  }
  /*--------------------------------------------------------------------------*/
  int  getXLocalDimension( ) {    return local_n ;  }
  /*--------------------------------------------------------------------------*/
  int  getYLocalDimension( ) {    return local_m ;  }
  /*--------------------------------------------------------------------------*/
  Handle<Options> **createSuperGlueHandles(){
    if ( hM ) 
      return hM;
    int nb=local_nb,mb=local_mb;
    hM= new Handle<Options>*[mb];
    for(int i=0;i<mb;i++){
      hM[i]=new Handle<Options>[nb];
    }   
    PRINT_IF(0)("memory size:%d, %d,%d\n",local_m * local_n,local_m,local_n);
    dtPartition->setBaseMemory( getContentAddress() , local_m * local_n);
    dtPartition->dump();
    dtPartition->partitionRectangle(local_m,local_n,local_mb,local_nb);
    PRINT_IF(0)("m:%d,n:%d,mb:%d,nb:%d\n",local_m,local_n,local_mb,local_nb);
    
    for(int i=0;i<mb;i++){
      for(int j=0;j<nb;j++){
	hM[i][j].block =dtPartition->getBlock(i,j);
	PRINT_IF(0)("Block(%d,%d).Mem:%p\n",i,j,hM[i][j].block->getBaseMemory());
	PRINT_IF(0)("Block(%d,%d).Y_E:%d,X_E:%d\n",i,j,hM[i][j].block->Y_E(),hM[i][j].block->X_E());
	PRINT_IF(0)("Block(%d,%d).Y_EB:%d,X_EB:%d\n",i,j,hM[i][j].block->Y_EB(),hM[i][j].block->X_EB());
	sprintf( hM[i][j].name,"M[%d,%d](%d,%d)",blk.by,blk.bx,i,j);
      }
    }
    return hM;
  }
  /*--------------------------------------------------------------------------*/
  bool isDataSent(int _host , DataVersion version){
    list<DataListener *>::iterator it;
    for (it = listeners.begin();it != listeners.end();it ++){
      DataListener *lsnr = (*it);
      if (lsnr->host == _host && lsnr->version == version){
	PRINT_IF(0)("DLsnr data:%s for host:%d is sent?:%d\n",name.c_str(),_host , lsnr->sent);
	version.dump();
	return  lsnr->sent;
      }
    }
    return false;
  }
  /*--------------------------------------------------------------------------*/
  void dataIsSent(int _host) {
    list<DataListener *>::iterator it;

    PRINT_IF(0)("Data:%s,DLsnr sent to host:%d,cur ver:\n",name.c_str(),_host);
    rt_write_version.dump();
    for (it = listeners.begin();it != listeners.end();it ++){
      DataListener *lsnr = (*it);
      if (lsnr->host == _host && lsnr->version == rt_write_version){
	PRINT_IF(0)("DLsnr rt_read_version before upgrade:\n");
	rt_read_version.dump();
	incrementRunTimeVersion(READ,lsnr->count);
	PRINT_IF(0)("DLsnr rt_read_version after upgrade:\n");
	rt_read_version.dump();
	lsnr->sent = true;
	//listeners.erase(it);
	return;
      }      
    }    
  }
  /*--------------------------------------------------------------------------*/
  void listenerAdded(int host , DataVersion version ) {
    list<DataListener *>::iterator it;
    version.dump();
    for (it = listeners.begin();it != listeners.end();it ++){
      DataListener *lsnr = (*it);
      if (lsnr->host == host && lsnr->version == version){
	lsnr->count ++;
	return;
      }
    }
    DataListener *lsnr = new DataListener;
    lsnr->host    = host ;
    lsnr->version = version ;
    lsnr->count   = 1;
    lsnr->sent = false;
    listeners.push_back(lsnr);
  }
  /*--------------------------------------------------------------------------*/
  DataVersion getRunTimeVersion(byte type){ 
    if ( type == IData::WRITE ) 
      return rt_read_version;
    else
      return rt_write_version;

  }
  /*--------------------------------------------------------------------------*/
  void setRunTimeVersion(string to_ctx, int to_version){
	TRACE_LOCATION;
    rt_read_version = to_version;
	TRACE_LOCATION;
    rt_read_version.setContext(to_ctx);
	TRACE_LOCATION;
    rt_write_version = to_version;
	TRACE_LOCATION;
    rt_write_version.setContext(to_ctx);
	TRACE_LOCATION;
  }
  /*--------------------------------------------------------------------------*/
  void deleteListenersForOldVersions(){
    list<DataListener *>::iterator it;
    it = listeners.begin();
    for (;it != listeners.end();){
      DataListener *lsnr = (*it);
      if (lsnr->version < rt_write_version ){
	PRINT_IF(0)("Data:%s,listeners deleted before:\n",name.c_str());
	rt_write_version.dump();
	listeners.erase(it);
      }
      else it ++;
    }
  }
  /*--------------------------------------------------------------------------*/
  void incrementRunTimeVersion(byte type,int v = 1 ){
    if ( type == IData::WRITE ) {
      rt_read_version += v;
      rt_write_version= rt_read_version;
      //deleteListenersForOldVersions();
      dump('W');
    }
    else{
    TRACE_LOCATION;
      rt_read_version +=v;
      dump('R');
    }
  }
  /*--------------------------------------------------------------------------*/
  int   getHost();
  /*--------------------------------------------------------------------------*/
  IData *operator () (const int i,const int j=0) {    
    return (*dataView)[i][j];  
  }
  /*--------------------------------------------------------------------------*/
  double getElement(int row , int col =0) {
    double *m = dtPartition->getElementAt(row,col);
    return *m;
  }
  /*--------------------------------------------------------------------------*/  
  void setElement(int row, int col ,double v) {
    TRACE_LOCATION;
    double *elem = dtPartition->getElementAt(row,col);
    *elem=v;
  }
  /*--------------------------------------------------------------------------*/  
  void dumpElements(){
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
  IData *getDataByHandle(DataHandle *in_dh ) ;  
  /*--------------------------------------------------------------------------*/
  byte *serialize(){
    int offset = 0 ;
    TRACE_LOCATION;
    byte * buffer =getHeaderAddress(); 
    serialize(buffer,offset,getHeaderSize() );
    return buffer;
  }
  /*--------------------------------------------------------------------------*/
  void serialize(byte *buffer , int &offset, int max_length){
    my_data_handle->serialize(buffer,offset,max_length);
    gt_read_version.serialize(buffer,offset,max_length);
    gt_write_version.serialize(buffer,offset,max_length);
    rt_read_version.serialize(buffer,offset,max_length);
    rt_write_version.serialize(buffer,offset,max_length);
  }
  /*--------------------------------------------------------------------------*/
  void deserialize(byte *buffer, int &offset,int max_length,MemoryItem *mi,bool header_only = true){
    my_data_handle->deserialize(buffer,offset,max_length);
    gt_read_version.deserialize(buffer,offset,max_length);
    gt_write_version.deserialize(buffer,offset,max_length);
    rt_read_version.deserialize(buffer,offset,max_length);
    rt_write_version.deserialize(buffer,offset,max_length);

    if ( !header_only ) {
      setDataMemory( mi ) ;
      dtPartition->setBaseMemory( getContentAddress(), getContentSize()) ; 
    }
  }
  /*--------------------------------------------------------------------------*/
  int getContentSize(){
    return content_size;
  }
  /*--------------------------------------------------------------------------*/
  int getHeaderSize(){
    return my_data_handle->getPackSize() + 
      4 * gt_read_version .getPackSize() ;
  }
  /*--------------------------------------------------------------------------*/
  int getPackSize(){ 
    return getHeaderSize() +  getContentSize() ;
  }
  /*--------------------------------------------------------------------------*/
  void testHandles();
  /*--------------------------------------------------------------------------*/
  bool isOwnedBy(int p ) ;
  /*--------------------------------------------------------------------------*/
  void incrementVersion ( AccessType a);
  /*--------------------------------------------------------------------------*/
  void dump(char c=' '){
    if ( c == 'N') 
      printf("#data%c: %11.11s \n",c,getName().c_str());
    if (!DUMP_FLAG)
      return;
    if ( c == ' ' ) 
      return;
    dumpVersion();
    dumpElements();
    //dtPartition->dumpBlocks();
  }
  /*--------------------------------------------------------------------------*/
  void dumpVersion(){
    if (!DUMP_FLAG)
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
  void addToVersion(AccessType axs,int v){
    gt_read_version += v;
    if ( axs == WRITE ) {
      gt_write_version = gt_read_version;
    }
    dump();
  }
  /*--------------------------------------------------------------------------*/
  void setPartition(int _mb, int _nb){
    
    Nb = _nb;
    Mb = _mb;
    dataView=new vector<vector<IData*> >  (Mb, vector<IData*>(Nb)  );
    char s[100];
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++){
	sprintf(s,"%s_%2.2d_%2.2d",  name.c_str() , i ,j);
	if ( Nb == 1) sprintf(s,"%s_%2.2d",  name.c_str() , i );
	if ( Mb == 1) sprintf(s,"%s_%2.2d",  name.c_str() , j );
	(*dataView)[i][j] = new IData (static_cast<string>(s),M/Mb,N/Nb,parent_context);
	IData *newPart = (*dataView)[i][j];
	newPart->blk.bx = j;
	newPart->blk.by = i;
	newPart->parent_data = this ;
	newPart->hpData = hpData ;
	TRACE_LOCATION;
	newPart->Nb = 0 ;
	newPart->Mb = 0 ;
	newPart->N  = N ;
	newPart->M  = M ;
	newPart->local_nb  = local_nb ;
	newPart->local_mb  = local_mb ;
	newPart->local_n = N / Nb   ;
	newPart->local_m = M / Mb   ;

	TRACE_LOCATION;
	newPart->allocateMemory();
	newPart->dtPartition=new Partition<double>(2);
	Partition<double> *p = newPart->dtPartition;
	TRACE_LOCATION;
	p->setBaseMemory(newPart->getContentAddress() ,  newPart->getContentSize());
	TRACE_LOCATION;
	p->partitionRectangle(newPart->local_m,newPart->local_n,
			      local_mb,local_nb);	
	TRACE_LOCATION;
	if ( newPart->getHost() != me ) {
	  TRACE_LOCATION;
	  newPart->setRunTimeVersion("-1",-1);
	  newPart->resetVersion();
	}
	TRACE_LOCATION;

      }
    TRACE_LOCATION;
  }
  /*--------------------------------------------------------------------------*/
  void resetVersion(){
    gt_read_version = gt_write_version = 0 ;
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)
	(*dataView)[i][j]->resetVersion();
  }

  /*--------------------------------------------------------------------------*/
  Coordinate  getBlockIdx(){ return blk;  }
  /*--------------------------------------------------------------------------*/
  DataRange  *RowSlice(int r , int i, int j ) {
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = r;
    dr->row_to   = r;
    dr->col_from = i;
    dr->col_to   = j;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  DataRange  *ColSlice(int c , int i, int j ) {
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = i;
    dr->row_to   = j;
    dr->col_from = c;
    dr->col_to   = c;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  DataRange  *Region(int fr, int tr, int fc, int tc ) {
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = fr;
    dr->row_to   = tr;
    dr->col_from = fc;
    dr->col_to   = tc;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  DataRange  *Cell(int i, int j ) {
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = i;
    dr->row_to   = i;
    dr->col_from = j;
    dr->col_to   = j;
    return dr;
  }
  /*--------------------------------------------------------------------------*/
  DataRange  *All(){
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = 0;
    dr->row_to   = Mb-1;
    dr->col_from = 0;
    dr->col_to   = Nb-1;
    return dr;
  }
};
/*========================== IData Class =====================================*/


class Data: public IData
{
public:
  Data(string  _name,int n, int m,IContext *ctx):  IData(_name,n,m,ctx){}
};

#endif //__DATA_HPP__
