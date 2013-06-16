#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <string>
#include <cstdio>

#include <vector>
#include <list>

#include "procgrid.hpp"
#include "hostpolicy.hpp"
#include "data.hpp"
#include "glb_context.hpp"

using namespace std;

GlobalContext glbCtx;
int me,version;

bool begin_context(IContext * curCtx,DataRange *r1,DataRange *r2,DataRange *w){
  ContextHeader *Summary = glbCtx.getHeader();
  Summary->addDataRange ( IData::READ,r1);
  if ( r2 != NULL ) 
    Summary->addDataRange ( IData::READ,r2);
  Summary->addDataRange ( IData::WRITE,w);
  bool b=glbCtx.getContextHostPolicy()->isAllowed(curCtx,Summary);
  if (!b){
      //  update versions of r1,r2,w
  }
  return b;

}
void end_context(IContext * curCtx){
}


void AddTask(char*s,IData *d1,IData *d2,IData *d3)
{

  glbCtx.incrementCounter(GlobalContext::TaskAdd);
  d1->incrementVersion(IData::READ);
  if ( d2 != NULL ) d2->incrementVersion(IData::READ);
  d3->incrementVersion(IData::WRITE);
  if ( d3->isOwnedBy(me) )
    glbCtx.incrementCounter(GlobalContext::TaskInsert);

}
void AddTask ( char*s,IData *d1)           { AddTask ( s,d1,NULL , d1);}
void AddTask ( char*s,IData *d1,IData *d2) { AddTask ( s,d1,NULL , d2);}

void AddTask_Debug(char*s,IData *d1,IData *d2,IData *d3)
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




class IContext;
class IData;

/*===============================  Context Class =======================================*/
class IContext
{
protected:
  string name;
  IContext *parent;
  list<IContext*> children;
  list<IData*> inputData,outputData;
  ProcessGrid *PG;
public:
  IContext(string _name){
    name=_name;
    parent =NULL;
  }
  string getName(){return name;}
  string getFullName(){return getName();}
  void print_name(const char *s=""){cout << s << name << endl;}
  virtual void downLevel(){}
  virtual void upLevel(){}

  void createPropagateTasks(){}
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
   ~IContext(){
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    //printf("Context Destructor,%s\n",name.c_str());
    for ( it =inputData.begin(); it != inputData.end(); ++ it ){
      delete (*it);
    }
    /*
    for ( it =outputData.begin(); it != outputData.end(); ++ it ){
      delete (*it);
    }
    */
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      delete (*ctx_it);
    }
    inputData.clear();
    outputData.clear();
    children.clear();
    //printf("dtor\n");
  }
  void resetVersions(){
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ;
    //printf("Context Reset Versions,%s\n",name.c_str());
    for ( it =inputData.begin(); it != inputData.end(); ++ it ){
      (*it)->resetVersion();
    }
    /*
    for ( it =outputData.begin(); it != outputData.end(); ++ it ){
      (*it)->resetVersion();
    }
    */
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it ){
      (*ctx_it)->resetVersions();
    }
    //printf("crvf\n");
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
  bool isAnyOwnedBy(list<DataRange*> dr,int me){
    list<DataRange*>::iterator  it = dr.begin();
    for ( ; it != dr.end() ;++it ){
      for ( int r = (*it)->row_from; r <=(*it)->row_to; r++){
	for ( int c = (*it)->col_from; c <=(*it)->col_to; c++){
	  IData &d=*((*it)->d);
	  bool b = d(r,c)->isOwnedBy(me) ;
	  if ( b )
	    return true;
	}
      }
    }
    return false;
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
/*=======================================================================*/
bool IData::isOwnedBy(int p ) {
  if (parent_data->Nb == 1 ||   parent_data->Mb == 1 ) {
    Coordinate c = blk;
    if ( parent_data->Nb ==1) c.bx = c.by;
    printf("blk: %d,%d\n",c.bx,c.by);
    return ( hpData->getHost ( c,1 ) == p );
  }
  else
    return ( hpData->getHost ( blk ) == p );
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
	(*dataView)[i][j]->changeContext(me);

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
  }
/*=======================================================================*/
bool GlobalContext::Boundry(IContext* ctx,bool c){
    if (!c) {
      // Propagte Versions
      ctx->changeContext(me);
      printf("Propagate, %d.x->%d.0\n",s,s+1);
    }
    s++;
    return c;
  }
bool GlobalContext::BoundryWithHeader(IContext* ctx,ContextHeader *summary){
  s++;
  glbCtx.incrementCounter(Contexts);
  if ( ctx->isAnyOwnedBy(summary->getWriteRange(), me) ) {
    glbCtx.incrementCounter(SummaryWrite);
    return true;
  }
  glbCtx.incrementCounter(SummaryRead);
  list <DataRange *>::iterator it;
  for ( it  = summary->getReadRange().begin();
	it != summary->getReadRange().end();
	++it )
  {
    IData &d = *((*it)->d);
    printf("Row Range: %d-%d,col Range: %d-%d\n",(*it)->row_from,(*it)->row_to,(*it)->col_from,(*it)->col_to);
    for (int r=(*it)->row_from;r <= (*it)->row_to; r++)
      for ( int c = (*it)->col_from; c <= (*it)->col_to; c++){
	if ( d(r,c)->isOwnedBy(me) ) {
	  d(r,c)->incrementVersion(IData::READ);
	  glbCtx.incrementCounter(VersionTrack);
	}
      }
  }

  return false;
}


void GlobalContext::dumpStatistics(Config *cfg){
  if ( me ==0)printf("@STAT:Node,Ctx,SmWr,SmRd,VerTr,T.Add,T.Ins,TotTask,Nb,P\n");
  printf("@STAT:%2d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
	  me,
	  counters[Contexts],
	  counters[SummaryWrite],
	  counters[SummaryRead],
	  counters[VersionTrack],
	  counters[TaskAdd],
	  counters[TaskInsert],
	 (me != -1)?-1:counters[TaskAdd],
	  cfg->getXBlocks(),
	  cfg->getProcessors());
  }
/*=======================================================================*/
bool ContextHostPolicy ::isAllowed(IContext *ctx,ContextHeader *hdr){
  if (active_policy == PROC_GROUP_CYCLIC){
    int ctx_group = glbCtx.getID() % PG->getGroupCount();
    int my_group = me / PG->getProcessorCount();
    return (ctx_group == my_group ) ;
  }
  return false;
}

#endif //  __CONTEXT_HPP__
