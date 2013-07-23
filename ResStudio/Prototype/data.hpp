#ifndef __DATA_HPP__
#define __DATA_HPP__

#include <string>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include "basic.hpp"

using namespace std;

class IData;
class IHostPolicy ;

struct  ContextPrefix{
  list<int> context_id_list;
  ContextPrefix(){
  }
  ContextPrefix(string s){
    fromString(s);
  }
  ~ContextPrefix(){
    reset();
  }
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
  bool operator !=(ContextPrefix rhs){
    return !(*this == rhs);
  }
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
  ContextPrefix operator=(ContextPrefix rhs){
    reset();
    context_id_list.assign( rhs.context_id_list.begin(),rhs.context_id_list.end());
    return *this;
  }
  string toString(){
    list<int>::iterator it;
    ostringstream s;
    for(it = context_id_list.begin(); it != context_id_list.end(); it ++){      
      s << (*it) << ".";
    }
    return s.str();      
  }
  void dump(){
    printf("prefix: %s\n",toString().c_str());
  }
  void reset(){
    context_id_list.clear();
  }
};
struct DataVersion{
public:  
  ContextPrefix prefix;
  int version;
  int getVersion(){
    return version;
  }
  DataVersion(){
    version =0 ;
  }
  ~ DataVersion(){
  }
  DataVersion operator +=(int a){
    version += a;
    return *this;
  }
  DataVersion operator =(DataVersion rhs){
    version = rhs.version;
    prefix  = rhs.prefix ;
    return *this;
  }
  DataVersion operator =(int a){
    version = a;
    return *this;
  }
  bool operator ==(DataVersion rhs){
    if (prefix != rhs.prefix) {
      return false;
    }
    if ( version == rhs.version){
      return true;
    }
    return false;
  }
  bool operator !=(DataVersion &rhs){
    return !(*this==rhs);
  }
  DataVersion operator ++(int a) {
    version ++;
    return *this;
  }
  DataVersion operator ++() {
    version ++;
    return *this;    
  }
  string dumpString(){
    ostringstream os;
    os << prefix.toString()  << version;
    return os.str();
  }
  void dump(){
    printf("version: %s\n",dumpString().c_str());
  }
  int   serialize(byte *buffer,int &offset,int max_length){
    copy<int>(buffer,offset,version);
    prefix.serialize(buffer,offset,max_length);
  }
  int deserialize(byte *buffer,int &offset,int max_length){
    paste<int>(buffer,offset,&version);
    prefix.deserialize(buffer,offset,max_length);
  }
  void reset(){
    prefix.reset();
    version =0 ;
  }
  void setContext(string s ){
    prefix.fromString(s);
  }
};

class Coordinate
{
public :
  int by,bx;
  Coordinate(int i , int j ) : by(i),bx(j){}
  Coordinate( ) : by(0),bx(0){}
};


typedef struct {
  IData *d;
  int row_from,row_to,col_from,col_to;
}DataRange;


struct DataHandle{
public:
  unsigned long int context_handle,data_handle;
  DataHandle () {context_handle=data_handle=0;}
  DataHandle (unsigned long int c,unsigned long int d ){
    context_handle = c;
    data_handle = d;
  }
  
  DataHandle operator = ( DataHandle rhs ) {
    this->context_handle = rhs.context_handle ;
    this->data_handle   = rhs.data_handle    ;
  }
  int    serialize(byte *buffer, int &offset,int max_length){
    copy<unsigned long>(buffer,offset,context_handle);
    copy<unsigned long>(buffer,offset,   data_handle);
  } 
  void deserialize(byte *buffer, int &offset,int max_length){
    paste<unsigned long>(buffer,offset,&context_handle);
    paste<unsigned long>(buffer,offset,   &data_handle);
  }
};
class IContext;
/*================== Data Class =====================*/
class IData
{
protected:
  string                      name;
  int                         N,M,Nb,Mb;
  Coordinate                  blk;
  vector< vector<IData*> >   *dataView;
  DataVersion                 current_version,request_version;
  DataVersion                 rt_read_version,rt_write_version;
  IContext                   *parent_context;
  IData                      *parent_data;
  IHostPolicy                *hpData;
  DataHandle                 *my_data_handle;
public:
  enum AccessType {
    READ  = 1,
    WRITE = 2
  };
  IData(){
    my_data_handle = new DataHandle;
  }
   IData(string _name,int m, int n,IContext *ctx);
  ~IData() {
  }

  string        getName          ()                { return name;}
  IContext     *getParent        ()                { return parent_context;}
  IData        *getParentData    ()                { return parent_data;}
  void          setParent        (IContext *p)     { parent_context=p;}
  DataVersion & getRequestVersion()                { return request_version;}
  DataVersion & getCurrentVersion()                { return current_version;}
  IHostPolicy  *getDataHostPolicy()                { return hpData;}
  void          setDataHostPolicy(IHostPolicy *hp) { hpData=hp;}
  void          setDataHandle    ( DataHandle *d)  { my_data_handle = d;}
  DataHandle   *getDataHandle    ()                { return my_data_handle ; }

  DataVersion getRunTimeVersion(byte type){ 
    if ( type == IData::WRITE ) 
      return rt_read_version;
    else
      return rt_write_version;

  }
  void setRunTimeVersion(string to_ctx, int to_version){//todo
    rt_read_version = to_version;
    rt_read_version.setContext(to_ctx);
    rt_write_version = to_version;
    rt_write_version.setContext(to_ctx);
  }
  void incrementRunTimeVersion(byte type ){
    if ( type == IData::WRITE ) {
      rt_write_version= ++rt_read_version;
      dump('W');
    }
    else{
      rt_read_version++;
      dump('R');
    }
  }


  int   getHost();

  IData *operator () (const int i,const int j=0) {    return (*dataView)[i][j];  }
  IData *getDataByHandle(DataHandle *in_dh ) ;
  
  void serialize(byte *buffer , int &offset, int max_length){
    my_data_handle->serialize(buffer,offset,max_length);
    current_version.serialize(buffer,offset,max_length);
    request_version.serialize(buffer,offset,max_length);
    rt_read_version.serialize(buffer,offset,max_length);
    rt_write_version.serialize(buffer,offset,max_length);
    // todo :content of data
  }
  void deserialize(byte *buffer, int &offset,int max_length,bool header_only = true){
    my_data_handle->deserialize(buffer,offset,max_length);
    current_version.deserialize(buffer,offset,max_length);
    request_version.deserialize(buffer,offset,max_length);
    rt_read_version.deserialize(buffer,offset,max_length);
    rt_write_version.deserialize(buffer,offset,max_length);
    if ( !header_only ) {
    // todo content of data
    }
  }
  int getPackSize(){ return 1024;}//Todo
  void testHandles();
  bool isOwnedBy(int p ) ;
  void incrementVersion ( AccessType a);
  void dump(char c=' '){
    printf("#data%c: %11.11s ",c,getName().c_str());
    dumpVersion();
  }
  void dumpVersion(){

    printf("\t\t%s\t\t%s\t\t%s\t\t%s\n",
	   current_version.dumpString().c_str(),
	   request_version.dumpString().c_str(),
	   rt_read_version.dumpString().c_str(),
	   rt_write_version.dumpString().c_str());
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)  {
	(*dataView)[i][j]->dumpVersion();
      }
  }
  void addToVersion(AccessType axs,int v){
    current_version += v;
    if ( axs == WRITE ) {
      request_version = current_version;
    }
    dump();
  }
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
	(*dataView)[i][j]->blk.bx = j;
	(*dataView)[i][j]->blk.by = i;
	(*dataView)[i][j]->parent_data = this ;
	(*dataView)[i][j]->hpData = hpData ;
      }
  }
  void resetVersion(){
    current_version = request_version = 0 ;
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)
	(*dataView)[i][j]->resetVersion();
  }

  Coordinate  getBlockIdx(){ return blk;  }
  DataRange  *RowSlice(int r , int i, int j ) {
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = r;
    dr->row_to   = r;
    dr->col_from = i;
    dr->col_to   = j;
    return dr;
  }
  DataRange  *ColSlice(int c , int i, int j ) {
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = i;
    dr->row_to   = j;
    dr->col_from = c;
    dr->col_to   = c;
    return dr;
  }
  DataRange  *Region(int fr, int tr, int fc, int tc ) {
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = fr;
    dr->row_to   = tr;
    dr->col_from = fc;
    dr->col_to   = tc;
    return dr;
  }
  DataRange  *Cell(int i, int j ) {
    DataRange *dr = new DataRange;
    dr->d = this;
    dr->row_from = i;
    dr->row_to   = i;
    dr->col_from = j;
    dr->col_to   = j;
    return dr;
  }
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


class Data: public IData
{
public:
  Data(string  _name,int n, int m,IContext *ctx):  IData(_name,n,m,ctx){}
};

#endif //__DATA_HPP__
