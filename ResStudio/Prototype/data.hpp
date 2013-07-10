#ifndef __DATA_HPP__
#define __DATA_HPP__

#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include "basic.hpp"

using namespace std;

class IData;
class IHostPolicy ;

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

typedef struct {
  int cur_version,req_version,ctx_switch;
}DataVersions;
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
    printf("buf:%p,ofs:%d,len:%d\n",buffer,offset,max_length);
    flushBuffer(buffer,max_length);
    paste<unsigned long>(buffer,offset,&context_handle);
    paste<unsigned long>(buffer,offset,   &data_handle);
    printf("ctx hdl:%ld,dt-hdl:%ld\n",context_handle,data_handle);
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
  list<DataVersions>          versions_track;
  unsigned int                current_version,request_version,rt_read_version,rt_write_version;
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
  int           getRequestVersion()                { return request_version;}
  int           getCurrentVersion()                { return current_version;}
  IHostPolicy  *getDataHostPolicy()                { return hpData;}
  void          setDataHostPolicy(IHostPolicy *hp) { hpData=hp;}
  void          setDataHandle    ( DataHandle *d)  { my_data_handle = d;}
  DataHandle   *getDataHandle    ()                { return my_data_handle ; }

  //  void          setRunTimeVersion(int v )          { runtime_version = v ; }
  int           getRunTimeVersion(byte type){ 
    if ( type == IData::WRITE ) 
      return rt_read_version;
    else
      return rt_write_version;

  }
  void          incrementRunTimeVersion(byte type ){
    if ( type == IData::WRITE ) {
      rt_write_version= rt_read_version++;
    }
    else{
      rt_read_version++;
    }
  }


  int   getHost();

  IData *operator () (const int i,const int j=0) {    return (*dataView)[i][j];  }
  IData *getDataByHandle(DataHandle *in_dh ) ;
  
  void serialize(byte *buffer , int &offset, int max_length){//toDo
    my_data_handle->serialize(buffer,offset,max_length);
    copy<unsigned int>(buffer,offset,current_version);
    copy<unsigned int>(buffer,offset,request_version);
    copy<unsigned int>(buffer,offset,rt_read_version);
    copy<unsigned int>(buffer,offset,rt_write_version);
    // content of data
  }
  void deserialize(byte *buffer, int &offset,int max_length,bool header_only = true){//todo
    TRACE_LOCATION;
    my_data_handle->deserialize(buffer,offset,max_length);
    TRACE_LOCATION;
    paste<unsigned int>(buffer,offset,&current_version);
    paste<unsigned int>(buffer,offset,&request_version);
    paste<unsigned int>(buffer,offset,&rt_read_version);
    paste<unsigned int>(buffer,offset,&rt_write_version);
    if ( !header_only ) {
    // content of data
    }
  }
  int getPackSize(){ return 1024;}//Todo
  void testHandles();
  bool isOwnedBy(int p ) ;
  void incrementVersion ( AccessType a);
  void dumpVersion(){
    printf("   @@%s,%d-%d\n",getName().c_str(),getCurrentVersion(),getRequestVersion());
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)  {
          //printf("----%s\n",(*dataView)[i][j]->getName().c_str());
	(*dataView)[i][j]->dumpVersion();
	  }
  }
  void addToVersion(AccessType axs,int v){
    current_version += v;
    if ( axs == WRITE ) {
      request_version = current_version;
    }
    printf("version jumped %s,%d, reqv=%d,curv=%d\n",getName().c_str(),v, request_version ,current_version);
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
