#ifndef __GENERIC_CONTEXTS_HPP
#define __GENERIC_CONTEXTS_HPP


#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string>
#include <assert.h>
#include <list>
#include <vector>
#include <sstream>
#include <stack>

using namespace std;

#define BX(a)  GenericContext *a=new GenericContext(string(#a),string(__func__),__LINE__);\
  if ( ctx_enter(a)) { 

#define BXX(a,x) GenericContext *a=new GenericContext(string(#a),string(__func__),__LINE__,x); \
  if ( ctx_enter(a)) { 


#define EX(a) } a->EndContext();

#define PRINTF_IND Glb.print_indent();printf


int procs;
extern int me;
class GenericContext;
bool ctx_enter(GenericContext *x,int p =-2);

/*===================================================================================*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*===================================================================================*/
struct ContextNode{
  GenericContext *ctx;
  bool begin;
  ContextNode(GenericContext *xx,bool b):
    ctx(xx),begin(b){}
};
typedef list<ContextNode *>::iterator CtxIterator;
typedef list<ContextNode *>::reverse_iterator CtxRIterator;
typedef vector<int> NodeList;
typedef NodeList::iterator NodeListIterator;
typedef list<IData*> DataList;
typedef DataList::iterator DListIter;
typedef stack<ContextNode *> ContextStack;


/*===================================================================================*/
/*====                                                                           ====*/
/*====                         Context Manager                                   ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*===================================================================================*/
class ContextManager{
private:
  list<ContextNode *> ctx_list;
  ContextStack ctx_stack;
public:
  void addCtx(GenericContext *x,bool begin);
  void dump();
  void reset();
  void process();    
  void processAll();    
  void sendCtxConnection(ContextNode *,ContextNode* , NodeList,NodeList);
  string getCurrentCtx();
  void dataTouched(IData *);
  ContextNode *getActiveCtx(){return ctx_stack.top();}
  void printTouchedData();
  ContextNode *getParentCtx();
  void sendContextInfo(string from, string to,DHList *dh,int dest);
  void recvContextInfo(char *buffer,int len);
  void insertRecvContext(char *,char * ,DHList );



};
ContextManager ctx_mngr;


/*===================================================================================*/
/*====                                                                           ====*/
/*====                         Generic Context                                   ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*===================================================================================*/
class GenericContext{
private:
  string name,func;
  int id,line,extra,level;
  NodeList entered_nodes,skipped_nodes;
  DataList touched_data;
public:
  static int last_id, indent;
/*------------------------------------------------------------*/
  GenericContext(string name_, string func_,int line_,int ex) {
    extra = ex;
    name = string(name_);
    func = string(func_);
    id = last_id++;
    line = line_;
    indent++;
    level = indent;
    who_enters();
    ctx_mngr.addCtx(this,true);
    print_indent();
    printf ("%s{  %s  line:%d , ex:%d .",
	    name.c_str(),func.c_str(),line,extra);
    print_entered_nodes();
  }
/*------------------------------------------------------------*/
  GenericContext(string name_, string func_,int line_) {
    extra = 0 ;
    name = string(name_);
    func = string(func_);
    id = last_id++;
    line = line_;
    indent++;
    level =indent;
    who_enters();
    ctx_mngr.addCtx(this,true);
    print_indent();
    printf ("%s{  %s  line:%d .",name.c_str(),func.c_str(),line);
    print_entered_nodes();
  }
/*------------------------------------------------------------*/
  GenericContext(char *unique_id){
    char *f=strtok(unique_id,"/");
    if ( f !=NULL ) {
      func.assign( string(f));
    }
    char *n=strtok(NULL,"/");
    if ( n!=NULL){
      name.assign(string(n));
    }
    char *l=strtok(NULL,"/");
    if ( l!=NULL){
      line=atoi(l);
    }
    char *ex=strtok(NULL,"/");
    if ( ex != NULL){
      extra=atol(ex);
    }
    id = last_id++;    
    indent++;
    level =indent;
    //    global=true;

  }
/*------------------------------------------------------------*/
  void dataTouched(IData *d);
  DHList * printTouchedData();
/*------------------------------------------------------------*/
  void EndContext(){
    ctx_mngr.addCtx(this,false);
    print_indent();
    printf ("}\n");
    indent--;
  }
/*------------------------------------------------------------*/
  ~GenericContext(){
  }
/*------------------------------------------------------------*/
  void who_enters(){

    for ( int p =0 ; p < procs;p++){
      if (ctx_enter(this,p)){
	entered_nodes.push_back(p);
      }
      else{
	skipped_nodes.push_back(p);
      }
    }
  }
/*------------------------------------------------------------*/
  void print_entered_nodes(){
    NodeListIterator it;
    printf ("(");
    for ( it = entered_nodes.begin(); 
	  it != entered_nodes.end(); it ++){
      printf ("%d,",*it);
    }
    printf (")\n");
  }
/*------------------------------------------------------------*/
  NodeList  &getExcludedNodes(){
    return skipped_nodes;
  }
/*------------------------------------------------------------*/
  NodeList  &getIncludedNodes(){
    return entered_nodes;
  }
/*------------------------------------------------------------*/
  int getID(){return id;}
/*------------------------------------------------------------*/
  int getLevel(){return level;}
/*------------------------------------------------------------*/
  string getUniqueID(){
    stringstream ss;
    ss << func << '/' << name << '/' << line << '/' << extra;
    return ss.str(); 
  }
/*------------------------------------------------------------*/
  int getLine(){return line;}
/*------------------------------------------------------------*/
  string  getName(){return name;}
/*------------------------------------------------------------*/
  string  getFunc(){return func;}
/*------------------------------------------------------------*/
  int getExtraID(){return extra;}
/*------------------------------------------------------------*/
  void resetCtx(){
    last_id = indent =1;
  }
/*------------------------------------------------------------*/
  void print_indent(){
    for (int i=0;i<indent;i++)printf("  ");
  }
/*------------------------------------------------------------*/
};

/*===================================================================================*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*===================================================================================*/
int GenericContext::last_id = 0;
int GenericContext::indent  = 0;
GenericContext Glb(string("Glb"),string("global"),0);



/*===================================================================================*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*===================================================================================*/

void ContextManager::dump()
{
  CtxIterator it;
  for ( it = ctx_list.begin();
	it != ctx_list.end();it++){
    ContextNode *gctx= *it;
    printf("[%d]",me);
    for (int i=0;i<gctx->ctx->getLevel();i++)printf("  ");
    printf("%s%c",gctx->ctx->getUniqueID().c_str(),
	   gctx->begin?'{':'}');
    gctx->ctx->print_entered_nodes();

  }
  
}
/*------------------------------------------------------------*/
void ContextManager::process()
{
  CtxRIterator it,pit;
  if (ctx_list.size()<=1)
    return;
  pit = ctx_list.rbegin();
  it = pit;
  pit++;
  ContextNode *ctxnode= *it;
  GenericContext *gctx=ctxnode->ctx;
  NodeList ex=gctx->getExcludedNodes();
  NodeList in=gctx->getIncludedNodes();
  if (ex.size() ==0 || gctx->getID() ==0){
    return;
  }
  NodeList pex= (*pit)->ctx->getExcludedNodes();
  if ( pex.size() ==0 ) {
    pit = it;
    return;
  }
  sendCtxConnection(*pit,*it,ex,in);
  
}
/*------------------------------------------------------------*/
void ContextManager::processAll()
{
  CtxIterator it,pit;
  pit = ctx_list.begin();
  for ( it = ctx_list.begin();
	it != ctx_list.end();it++){
    ContextNode *ctxnode= *it;
    GenericContext *gctx=ctxnode->ctx;
    NodeList ex=gctx->getExcludedNodes();
    NodeList in=gctx->getIncludedNodes();
    if (ex.size() ==0 || gctx->getID() ==0)
      continue;
    NodeList pex= (*pit)->ctx->getExcludedNodes();
    if ( pex.size() ==0 ) {
      pit = it;
      continue;
    }
    sendCtxConnection(*pit,*it,ex,in);
    pit = it;
  }
  
}
/*------------------------------------------------------------*/
void ContextManager::reset(){
    ctx_list.clear();
  }
/*------------------------------------------------------------*/
void ContextManager::addCtx(GenericContext *x,bool begin){
  //  if ( x->getID() ==0 )
  //    return;
  ContextNode *cn = new ContextNode(x,begin);
  if ( begin ){
    ctx_stack.push( cn );
  }
  else{
    ctx_stack.pop( );
  }
  ctx_list.push_back(cn);
  process();
}
/*------------------------------------------------------------*/
void ContextManager::sendCtxConnection(ContextNode *prv_ctx ,
				       ContextNode *cur_ctx,
				       NodeList ex,
				       NodeList in){
  NodeListIterator it;
  int p,m,n;
  string f,t;
  DHList *touched;
  n=in.size();
  m=ex.size();
  it = find(in.begin(),in.end(),me);
  p  = it - in.begin();
  
  for ( int i = p; i < m ; i +=n){
    f=prv_ctx->ctx->getUniqueID() + (prv_ctx->begin?'{':'}');
    t=cur_ctx->ctx->getUniqueID() + (cur_ctx->begin?'{':'}');
    printf("\t\t\t\t\t\t\t");
    printf("[%d] send %s->%s to %d",ex[i],f.c_str(),t.c_str(),ex[i]);    
  
    if ( !prv_ctx->begin && cur_ctx->begin){
      printf("  Ctx:%s",getParentCtx()->ctx->getName().c_str());
      touched=getParentCtx()->ctx->printTouchedData();
    }else{
      if ( prv_ctx->ctx !=NULL){
	touched=prv_ctx->ctx->printTouchedData();
      }
    }
    printf("\n");
    sendContextInfo(f,t,touched,ex[i]);
  }
}
/*------------------------------------------------------------*/
void ContextManager::sendContextInfo(string from, 
				     string to,
				     DHList *dh,
				     int dest){
  stringstream buffer;
  DHLIter it;
  buffer << from << ':' << to <<':';
  for ( it = dh->begin();it != dh->end(); it++)    {
    buffer << (*it).data_handle <<';';
  }
  cout << "*** "<<buffer.str() << "***"<<buffer.str().size() <<endl; 
  
  int len = from.length() + 1 +to.length() + 1+dh->size() * sizeof((DataHandle::data_handle)+1);
  cout << "len===" << len << endl;
  char * buf = new char[len];
  sprintf(buf,"%s:%s:",from.c_str(),to.c_str());
  
  for ( it = dh->begin();it != dh->end(); it++)    {
    sprintf(buf+strlen(buf) ,"%ld;", (*it).data_handle );
  }

  cout << buf<< ":" << strlen(buf) << endl;
  
  dtEngine.sendContextInfo(buf,len,dest);
  
  recvContextInfo(buf,len);
}
/*------------------------------------------------------------*/
void ContextManager::recvContextInfo(char *buffer,int len){
  char *f=strtok(buffer,":");
  if ( f == NULL ) {
    printf("error in from context string.\n");
    return ;
  }
  cout << "-->" << f << endl;
  char *t=strtok(NULL,":");
  if ( t == NULL ) {
    printf("error in to  context string.\n");
    return ;
  }
  cout << "-->" << t << endl;
  cout << "buf+:"  << (buffer+strlen(f)+strlen(t)+2) << endl;
  DHList *dhl=new DHList;
  char *d=strtok(buffer+strlen(f)+strlen(t)+2,";");
  while ( d != NULL ){
    cout << d << endl;
    DataHandle dh;
    dh.data_handle = atol(d);
    dhl->push_back(dh);
    d= strtok(NULL,";");
  }
} 
/*------------------------------------------------------------*/
void ContextManager::insertRecvContext(char *from,char *to,DHList dhl){
  string f(from),t(to);
  CtxIterator  it;
  bool found=false;
  bool fb = from[strlen(from)-1] =='{';
  bool tb = to[strlen(to)-1] =='{';
  ContextNode * cnf = new ContextNode (new GenericContext(from),fb);
  ContextNode * cnt = new ContextNode (new GenericContext(to),tb);
  //todo : get fom dhl the IData * list and add to cnt
  for ( it = ctx_list.begin();it != ctx_list.end();it++){
    ContextNode *cn = *it;
    string s=cn->ctx->getUniqueID()+(cn->begin?'{':'}');
    if ( f == s ) {
      ++it;
      ctx_list.insert(it,cnt);
      found=true;
      break;
    }
  }
  if (!found){
    ctx_list.push_back(cnf);
    ctx_list.push_back(cnt);
  }
}
/*------------------------------------------------------------*/
ContextNode *ContextManager::getParentCtx(){
  ContextNode *cn=getActiveCtx();
  if (ctx_stack.size() ==1 ){
    return cn;
  }
  ctx_stack.pop();
  ContextNode *cn2=getActiveCtx();
  ctx_stack.push(cn);
  return cn2;
}
/*------------------------------------------------------------*/
string ContextManager::getCurrentCtx(){
  if ( ctx_list.size()==0)
    return string("Glb{");
  CtxRIterator cx=ctx_list.rbegin();
  if ( (*cx)->ctx == NULL)
    return string("Glb{");
  return (*cx)->ctx->getUniqueID() + ( (*cx)->begin?'{':'}');
}

/*==================================================================*/
/*------------------------------------------------------------*/
void GenericContext::dataTouched(IData *d){
  touched_data.push_back(d);// to do : push data items only once
}
/*------------------------------------------------------------*/
DHList * GenericContext::printTouchedData(){
  DListIter it;
  DHList *dhl = new DHList;
  printf  ("   Touched Data : ");
  for(it =touched_data.begin(); 
      it!=touched_data.end();   it++){
    IData *d=*it;
    printf("%s,",d->getName().c_str());
    dhl->push_back(*d->getDataHandle());
  }
  touched_data.clear();
  return dhl;
}
/*------------------------------------------------------------*/
void ContextManager::dataTouched(IData *d){
  ContextNode *cn=getActiveCtx();
  if ( cn == NULL ) {
    printf("ActvCtx NULL \n");
    return;
  }
  cn->ctx->dataTouched(d);
}
/*------------------------------------------------------------*/
void ContextManager::printTouchedData(){
  ContextNode *cn=getActiveCtx();
  //  printf("00\n");
  if ( cn == NULL ) {
    printf("ActvCtx NULL \n");
    return;
  }
  printf("ctx:%s ",cn->ctx->getName().c_str());
  if ( cn->ctx ==NULL){
    return;
  }
  cn->ctx->printTouchedData();
}
string GlobalContext::getLevelString(){
    if (1){
      list <LevelInfo>::iterator it;
      ostringstream _s;
      for ( it = lstLevels.begin(); it != lstLevels.end(); ++it ) {
	_s << (*it).seq <<".";
      }
      return _s.str();
    }
    else{
      string s =  ctx_mngr.getCurrentCtx();
      cout << "===> "<<s << endl;
      return s;
    }
  }


#endif // __GENERIC_CONTEXTS_HPP

