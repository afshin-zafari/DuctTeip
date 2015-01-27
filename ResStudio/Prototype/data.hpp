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

/*===================ContextPrefix============================================*/
struct  ContextPrefix{
  list<int> context_id_list;
  ContextPrefix(){  }
  ContextPrefix(string s){    fromString(s);  }
  ~ContextPrefix(){    reset();  }
  /*--------------------------------------------------------------------------*/
  int getPackSize(){    return 10* sizeof(int);  }
  /*--------------------------------------------------------------------------*/
  int serialize(byte *buffer,int &offset,int max_length){
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
/*==========================ContextPrefix=====================================*/

/*==========================DataVersion ======================================*/
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
    os << prefix.toString()  << version;
    return os.str();
  }
  /*--------------------------------------------------------------------------*/
  void dump(char c=' '){
    if (!DUMP_FLAG)
      return;
    printf(",version:%s,\n",dumpString().c_str());
  }
  /*--------------------------------------------------------------------------*/
  int   serialize(byte *buffer,int &offset,int max_length){
    copy<int>(buffer,offset,version);
    prefix.serialize(buffer,offset,max_length);
    return 0;
  }
  /*--------------------------------------------------------------------------*/
  int deserialize(byte *buffer,int &offset,int max_length){
    paste<int>(buffer,offset,&version);
    prefix.deserialize(buffer,offset,max_length);
    return 0;
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
/*==========================DataVersion ======================================*/

/*============================================================================*/
class Coordinate
{
public :
  int by,bx;
  Coordinate(int i , int j ) : by(i),bx(j){}
  Coordinate( ) : by(0),bx(0){}
};//Coordinate

/*============================================================================*/

typedef struct {
  IData *d;
  int row_from,row_to,col_from,col_to;
}DataRange;


/*==========================DataHandle =======================================*/
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
  
  DataHandle &operator = ( DataHandle rhs ) {
    this->context_handle = rhs.context_handle ;
    this->data_handle   = rhs.data_handle    ;
    return *this;
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
    return 0;
  } 
  /*--------------------------------------------------------------------------*/
  void deserialize(byte *buffer, int &offset,int max_length){
    paste<unsigned long>(buffer,offset,&context_handle);
    paste<unsigned long>(buffer,offset,   &data_handle);
  }
};
/*==========================DataHandle =======================================*/
/*==========================DataListener =====================================*/
/*
struct DataListener{
  DataVersion version;
  int host,count;
  bool sent;
};
*/
/*==========================DataListener =====================================*/

class IContext;
class IListener;
class IDuctteipTask;
class MailBox;
typedef IListener DataListener;

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
  list<IListener *>          listeners;
  list<IDuctteipTask *>      tasks_list;
  int                        local_n, local_m,local_nb,local_mb,content_size;
  Partition<double>          *dtPartition;
  Handle<Options>            **hM;
  bool                       partial;
  list <int>                exported_nodes;
public:
  /*--------------------------------------------------------------------------*/
  enum AccessType {    READ  = 1,    WRITE = 2  , SCALAR=64};
  /*--------------------------------------------------------------------------*/
  IData * clone(MemoryItem *mi){
    IData *d = new IData;
    bool dbg = false;
    *d=*this ;    
    if(dbg)    printf("Data %s\n",d->getName().c_str());
    if(dbg)    printf("loc mb:%d, nb:%d\n",d->local_mb,d->local_nb);
    if(dbg)    printf("loc m:%d, n:%d\n",d->local_m,d->local_n);
    d->setDataHandle(my_data_handle);
    d->hM = NULL;
    if(dbg)    printf ("CLONE d.dh=%ld,my.dh=%ld\n",(d->getDataHandle())->data_handle,my_data_handle->data_handle);
    d->setDataMemory(mi);
    if ( data_memory  == NULL ){
      if(dbg)      printf ("CLONE my.mem is NULL\n");
      d->Mb = d->Nb = 0;
      //d->allocateMemory();
      if(dbg)      printf ("CLONE d.mem is %p\n",d->data_memory);
    }    
    if(dbg)    printf ("CLONE d.mem=%p\n",d->getContentAddress());
    if (getContentSize()==0){
      long ds;      
      ds = local_n * local_m * sizeof(double);
      d->setContentSize(ds);
    }
    else 
      d->setContentSize(getContentSize());
    d->prepareMemory();
    if(dbg)    printf ("CLONE d.mem=%p d.size=%d\n",d->getContentAddress(),d->getContentSize());
    if ( data_memory !=NULL)
      if(dbg)printf ("      my.mem=%p\n",getContentAddress());
    d->createSuperGlueHandles();
    if(dbg)    printf ("CLONE d.hm=%p,my.hm=%p\n",d->hM,hM);
    return d;
  }
  /*--------------------------------------------------------------------------*/
  IData(){
    my_data_handle = new DataHandle;
    dtPartition = NULL; 
    hM = NULL;
    name="";
    data_memory=NULL;
  }
  /*--------------------------------------------------------------------------*/
   IData(string _name,int m, int n,IContext *ctx);
  /*--------------------------------------------------------------------------*/
  ~IData() {
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
  string        getName          ()                { return name;}
  IContext     *getParent        ()                { return parent_context;}
  IData        *getParentData    ()                { return parent_data;}
  void          setParent        (IContext *p)     { parent_context=p;}
  DataVersion  &getWriteVersion  ()                { return gt_write_version;}
  DataVersion  &getReadVersion   ()                { return gt_read_version;}
  IHostPolicy  *getDataHostPolicy()                { return hpData;}
  void          setDataHostPolicy(IHostPolicy *hp) { hpData=hp;}
  void          setDataHandle    ( DataHandle *d)  { my_data_handle = d;}
  DataHandle   *getDataHandle    ()                { return my_data_handle ; }  
  unsigned long getDataHandleID  ()                {return my_data_handle->data_handle;}
  /*--------------------------------------------------------------------------*/
  void allocateMemory();
  void prepareMemory();
  void addTask(IDuctteipTask *);
  void checkAfterUpgrade(list<IDuctteipTask *> &,MailBox *,char debug =' ');
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
    //data_memory->setState( MemoryItem::Ready) ; 
    data_memory = mi;
    if(0)printf("======%s\n",getName().c_str());
  }
  /*--------------------------------------------------------------------------*/
  MemoryItem *getDataMemory(){
    return data_memory;
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
  Handle<Options> **createSuperGlueHandles(bool force = false){
    bool dbg=!true;
    if ( !force && hM !=NULL ) 
      return hM;
    if(dbg)        printf ("SG handle for Data %s is created.\n",getName().c_str());
    int nb=local_nb,mb=local_mb;
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
  bool isDataSent(int _host , DataVersion version);
  /*--------------------------------------------------------------------------*/
  void dataIsSent(int _host) ;
  /*--------------------------------------------------------------------------*/
  void listenerAdded(DataListener *,int host , DataVersion version ) ;
  /*--------------------------------------------------------------------------*/
  DataVersion getRunTimeVersion(byte type){ 
    if ( type == IData::WRITE ) 
      return rt_read_version;
    else
      return rt_write_version;

  }
  /*--------------------------------------------------------------------------*/
  void setRunTimeVersion(string to_ctx, int to_version){
    rt_read_version = to_version;
    rt_read_version.setContext(to_ctx);
    rt_write_version = to_version;
    rt_write_version.setContext(to_ctx);
  }
  /*--------------------------------------------------------------------------*/
  void deleteListenersForOldVersions();
  /*--------------------------------------------------------------------------*/
  void incrementRunTimeVersion(byte type,int v = 1 ){
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
  int   getHost();
  /*--------------------------------------------------------------------------*/
  IData *operator () (const int i,const int j=0) {    
    return (*dataView)[i][j];  
  }
  IData *operator [] (const int i) {    
    return (*dataView)[i][0];  
  }
  IData *operator & (void) {    
    return this;
  }
  /*--------------------------------------------------------------------------*/
  double getElement(int row , int col =0) {
    double *m = dtPartition->getElementAt(row,col);
    return *m;
  }
  /*--------------------------------------------------------------------------*/  
  void setElement(int row, int col ,double v) {
    double *elem = dtPartition->getElementAt(row,col);
    *elem=v;
  }
  /*--------------------------------------------------------------------------*/  
  void dumpElements(){
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
  IData *getDataByHandle(DataHandle *in_dh ) ;  
  /*--------------------------------------------------------------------------*/
  byte *serialize(){
    int offset = 0 ;
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
      /*
      if (mi != data_memory){
	printf("!!!!!!!!!!!!\n");
	hM = NULL;
	createSuperGlueHandles();
	}
      */
      if ( mi != NULL){
	setDataMemory( mi ) ;
	if(0)printf("DataMem Changed  %s.\n",getName().c_str());
      }
      dtPartition->setBaseMemory( getContentAddress(), getContentSize()) ; 
    }
  }
  /*--------------------------------------------------------------------------*/
  int getContentSize(){
    return content_size;
  }
  /*--------------------------------------------------------------------------*/
  void setContentSize(long s){content_size =s;}
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
  void dump(char ch=' '){
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
  string  dumpVersionString(){
    string s;
    s = gt_read_version.dumpString() + " " + 
	   gt_write_version.dumpString()+ " " + 
	   rt_read_version.dumpString()+ " " + 
      rt_write_version.dumpString()+ " " ;
    return s;
  }
  /*--------------------------------------------------------------------------*/
  void dumpVersion(){
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
  void addToVersion(AccessType axs,int v){
    gt_read_version += v;
    if ( axs == WRITE ) {
      gt_write_version = gt_read_version;
    }
    dump();
  }
  /*--------------------------------------------------------------------------*/
  void setPartition(int _mb, int _nb);
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
  list<IListener *>          &getListeners(){return listeners;}
  list<IDuctteipTask *>      &getTasks(){return tasks_list;}
  /*--------------------------------------------------------------------------*/
  double dumpCheckSum(char c='i'){	
    return 0.0;
    if ( c != 'z' && c!= 'Z' && c!='R' && c!='F' && c!='S') 
      return 0.0;
    if (!data_memory)
      return 0.0;
    double *contents=getContentAddress();
    long size = (getContentSize())/sizeof(double);
    double sum = 0.0;
    for ( long i=0; i< size; i++)
      sum += contents[i];
    printf("@CheckSum %c , %s,%lf adr:%p len:%ld \n",c,getName().c_str(),sum,contents,size);
    //dumpVersion();
    return sum;
  }
  /*--------------------------------------------------------------------------*/
  bool isExportedTo(int p ) {
    list<int>::iterator it;
    it = find(exported_nodes.begin(),exported_nodes.end(),p);
    bool t=  it != exported_nodes.end();
    //printf("Data %s is already sent to %d(y/n)? %c\n",getName().c_str(),p,t?'y':'n');
    return t;

  }
  /*--------------------------------------------------------------------------*/
  void setExportedTo(int p){
    exported_nodes.push_back(p);
  }

};
/*========================== IData Class =====================================*/


class Data: public IData
{
public:
  Data(string  _name,int n, int m,IContext *ctx):  IData(_name,n,m,ctx){}
};

typedef list<IData *> DataList;
typedef DataList::iterator DLIter;


#endif //__DATA_HPP__
