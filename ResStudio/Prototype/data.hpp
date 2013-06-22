#ifndef __DATA_HPP__
#define __DATA_HPP__

#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>

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
class IContext;
/*================== Data Class =====================*/
class IData
{
protected:
  string name;
  int N,M,Nb,Mb;
  Coordinate blk;
  vector< vector<IData*> >  *dataView;
  list<DataVersions> versions_track;
  unsigned int  current_version,request_version;
  IContext *parent_context;
  IData *parent_data;
  IHostPolicy *hpData;
public:
  enum AccessType {
    READ=1,
    WRITE = 2
  };
  IData(string _name,int m, int n,IContext *ctx):
    M(m),N(n), parent_context(ctx){
    name=_name;
    current_version=request_version=0;
  }
  ~IData() {
  }
  string        getName          ()                {return name;}
  IContext     *getParent        ()                {return parent_context;}
  IData        *getParentData    ()                {return parent_data;}
  void          setParent        (IContext *p)     { parent_context=p;}
  int           getRequestVersion()                {return request_version;}
  int           getCurrentVersion()                {return current_version;}
  IHostPolicy  *getDataHostPolicy()                {return hpData;}
  void          setDataHostPolicy(IHostPolicy *hp) { hpData=hp;}

  int getHost();
  void  resetVersion(){
    current_version = request_version = 0 ;
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)
	(*dataView)[i][j]->resetVersion();
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
	//cout << (*dataView)[i][j]->getName()<< endl;
      }
    //cout << "end of partition\n"  ;
  }
  IData * operator () (const int i,const int j=0) {    return (*dataView)[i][j];  }
  Coordinate getBlockIdx(){ return blk;  }
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
  void addToVersion(int v){
    current_version += v;
    request_version += v;
  }

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
  DataRange *All(){
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
