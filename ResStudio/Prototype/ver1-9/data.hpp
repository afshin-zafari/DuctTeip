#ifndef __DATA_HPP__
#define __DATA_HPP__

#include <string>
#include <iostream>
#include <vector>
#include <list>

using namespace std;
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
  vector< vector<IData*> >  dataView;
  list<DataVersions> versions_track;
  unsigned int  current_version,request_version;
  IContext *parent_context;
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
  string getName(){return name;}
  IContext *getParent() {return parent_context;}
  void setParent(IContext *p) { parent_context=p;}
  int getRequestVersion(){return request_version;}
  int getCurrentVersion(){return current_version;}
  void resetVersion(){
    current_version = request_version = 0 ;
    printf("Data Reset Versions,%s\n",name.c_str());
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)
	dataView[i][j]->resetVersion();
  }

  void setPartition(int _mb, int _nb){
    Nb = _nb;
    Mb = _mb;
    vector<vector<IData*> >  dataView2(Mb, vector<IData*>(Nb)  );
    char s[100];
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++){
	sprintf(s,"%s_%2.2d_%2.2d",  name.c_str() , i ,j);
	if ( Nb == 1) sprintf(s,"%s_%2.2d",  name.c_str() , i );
	if ( Mb == 1) sprintf(s,"%s_%2.2d",  name.c_str() , j );
	dataView2[i][j] = new IData (static_cast<string>(s),M/Mb,N/Nb,parent_context);
	dataView2[i][j]->blk.bx = j;
	dataView2[i][j]->blk.by = i;
	//cout << dataView2[i][j]->getName()<< endl;
      }
    dataView = dataView2;
    //cout << "end of partition\n"  ;
  }
  IData * operator () (const int i,const int j=0) {    return dataView[i][j];  }
  Coordinate getBlockIdx(){ return blk;  }
  bool isOwnedBy(int p ) ;
  void incrementVersion ( AccessType a);
  void changeContext(int me );
  void dumpContextSwitches(){
    list<DataVersions>::iterator it;
    
    printf ("%s : \n ctx cur req\n",name.c_str() ) ;
    for (it = versions_track.begin();it != versions_track.end();++it){
      printf("%3d %3d %3d\n",
	     (*it).ctx_switch,
	     (*it).cur_version,
	     (*it).req_version);
    }
    printf("\n");
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)
	dataView[i][j]->dumpContextSwitches();
  }
};

class Data: public IData 
{
public:
  Data(string  _name,int n, int m,IContext *ctx):  IData(_name,n,m,ctx){}
};

#endif //__DATA_HPP__
