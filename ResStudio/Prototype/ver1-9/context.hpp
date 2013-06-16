#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <string>
#include <iostream>
#include <vector>
#include <list>

#include "procgrid.hpp"
#include "hostpolicy.hpp"
#include "data.hpp"
#include "ctx_bndry.hpp"

using namespace std;
ContextBoundry glbCtx;
int me,version;

void AddTask(char*s,IData *d1,IData *d2,IData *d3)
{
  if (version >= 5 ) cout << glbCtx.getID() << " : ";
  printf("%s %s %s %s",s,
	 d1->getName().c_str(),
	 d2 ==NULL?"  --  " :d2->getName().c_str(),
	 d3->getName().c_str() );

  if ( d3->isOwnedBy(me) ) {
    cout << " owner-of-WRITE data" ;
  }
  printf("\n");
  if (version >=6 ) {
    d1->incrementVersion(IData::READ);
    if ( d2 != NULL ) d2->incrementVersion(IData::READ);
    d3->incrementVersion(IData::WRITE);
  }
  if ( version >=7 ) {
    int c = glbCtx.getID();
    printf("                  %d,%d   %d,%d    %d,%d\n",
	   d1->getCurrentVersion(),d1->getRequestVersion(),
	   (d2==NULL)?-1:d2->getCurrentVersion(),(d2==NULL)?-1:d2->getRequestVersion(),
	   d3->getCurrentVersion(),d3->getRequestVersion()
	   );
  }
}


#define GETTER(a,b) int get##a(){return b;}
#define SETTER(a,b) void set##a(int p){b=p;}
#define PROPERTY(a,b) GETTER(a,b) \
  SETTER(a,b)

class Config
{
private:  
  int N,M,Nb,Mb,P,p,q;
public:
  Config ( int n,int m , int nb , int mb , int PP, int pp, int qq):
    N(n),M(m),Nb(nb),Mb(mb),P(PP),p(pp),q(qq){}
  PROPERTY(XDimension,N)
  PROPERTY(YDimension,M)
  PROPERTY(XBlocks,Nb)
  PROPERTY(YBlocks,Mb)
  PROPERTY(Processors,P)
  PROPERTY(P_pxq,p)
  PROPERTY(Q_pxq,q)
  
};

class IContext;
class IData;

/*================== Context Class =====================*/
class IContext
{
protected: 
  string name;
  IContext *parent;
  list<IContext*> children;
  list<IData*> inputData,outputData;
  DataHostPolicy *hpData;
public:
  IContext(string _name){
    name=_name;
    parent =NULL;
  }
  void print_name(const char *s=""){cout << s << name << endl;}
  void setParent ( IContext *_p ) {
    parent = _p;
    parent->children.push_back(this);
  }
  void traverse(){
    if ( parent != NULL )
      parent->print_name("-");
    else
      cout << "-" << endl;
    print_name("--");
    for (list<IContext *>::iterator it=children.begin();it !=children.end();it++)
      (*it)->print_name("---");
  }
  void addInputData(IData *_d){
    inputData.push_back(_d);
  }
  void addOutputData(IData *_d){
    outputData.push_back(_d);
  }
  IHostPolicy *getDataHostPolicy() {return hpData;}
  void setDataHostPolicy(DataHostPolicy *hp) { hpData=hp;}
  void changeContext(int me) {
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    for ( it =inputData.begin(); it != inputData.end(); ++ it ){
      (*it)->changeContext(me);
    }
    for ( it =outputData.begin(); it != outputData.end(); ++ it ){
      (*it)->changeContext(me);
    }
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      (*ctx_it)->changeContext(me);
    }
  }
  void resetVersions(){
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    printf("Context Reset Versions,%s\n",name.c_str());
    for ( it =inputData.begin(); it != inputData.end(); ++ it ){
      (*it)->resetVersion();
    }
    for ( it =outputData.begin(); it != outputData.end(); ++ it ){
      (*it)->resetVersion();
    }
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      (*ctx_it)->resetVersions();
    }
  }
  void dumpContextSwitches(){
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    printf("Dump Context Switches ,%s\n",name.c_str());
    for ( it =inputData.begin(); it != inputData.end(); ++ it ){
      (*it)->dumpContextSwitches();
    }
    for ( it =outputData.begin(); it != outputData.end(); ++ it ){
      (*it)->dumpContextSwitches();
    }
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      (*ctx_it)->dumpContextSwitches();
    }
  }
};

class Context: public IContext
{
public :
  Context (const char *s ) :  
  IContext(static_cast<string>(s))
  {
  }
  Context (void):IContext("") {
    name="";
  }
};

bool IData::isOwnedBy(int p ) {
    return ( parent_context->getDataHostPolicy()->getHost ( blk ) == p );
  }
bool ContextBoundry::Boundry(IContext* ctx,bool c){
    if (!c) {
      // Propagte Versions
      ctx->changeContext(me);
      printf("Propagate, %d.x->%d.0\n",s,s+1);
    }
    s++;
    return c;
  }

void IData::changeContext(int me ) {
    DataVersions  v;
    if (  isOwnedBy(me) ) {
      v.ctx_switch  =  glbCtx.getID();
      v.cur_version = current_version; 
      v.req_version = request_version;
      versions_track.push_back(v);
    }
    for ( int i=0;i<Mb;i++)
      for ( int j=0;j<Nb;j++)
	dataView[i][j]->changeContext(me);    
    
  }
void IData::incrementVersion ( AccessType a) {
    DataVersions v;
    if ( a == WRITE ) {
      request_version = current_version;
      if ( version == 9 ) {
	v.ctx_switch = glbCtx.getID();
	v.cur_version = current_version;
	v.req_version = request_version;      
	versions_track.push_back(v);
      }
    }
    current_version++;      
    if ( version == 8 ) {
      v.ctx_switch = glbCtx.getID();
      v.cur_version = current_version;
      v.req_version = request_version;      
      versions_track.push_back(v);
    }
    //printf("\n%s,%d-%d\n",name.c_str(),request_version, current_version);
  }


#endif //  __CONTEXT_HPP__
