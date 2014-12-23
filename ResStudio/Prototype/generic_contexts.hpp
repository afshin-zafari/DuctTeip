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


/*===================================================================================*/
/*====                                                                           ====*/
/*====                         Context Manager                                   ====*/
/*====                                                                           ====*/
/*====                                                                           ====*/
/*===================================================================================*/
class ContextManager{
private:
  list<ContextNode *> ctx_list;
  stack<ContextNode *> ctx_stack;
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
    print_indent();
    printf ("%s{  %s  line:%d , ex:%d .",
	    name.c_str(),func.c_str(),line,extra);
    who_enters();
    ctx_mngr.addCtx(this,true);
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
    print_indent();
    printf ("%s{  %s  line:%d .",name.c_str(),func.c_str(),line);
    who_enters();
    ctx_mngr.addCtx(this,true);
  }
/*------------------------------------------------------------*/
  void dataTouched(IData *d);
  void printTouchedData();
/*------------------------------------------------------------*/
  void EndContext(){
    print_indent();
    printf ("}\n");
    indent--;
    ctx_mngr.addCtx(this,false);
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
    print_entered_nodes();
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
  if (ex.size() ==0 || gctx->getID() ==0)
    return;
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
    if ( x->getID() ==0 ) 
      return;
     ContextNode *cn = new ContextNode(x,begin);
     if ( begin ){
       ctx_stack.push( cn );
     }
     else{
       printTouchedData();
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
  n=in.size();
  m=ex.size();
  for ( p=0;p<n;p++)
    if (in[p]==me)
      break;
  if(1)
  for ( int i = p; i < m ; i +=n){
    printf("\t\t\t\t\t\t\t");
    printf("[%d] send %s%c->%s%c to %d\n",
	   ex[i],
	   prv_ctx->ctx->getUniqueID().c_str(),prv_ctx->begin?'{':'}',
	   cur_ctx->ctx->getUniqueID().c_str(),cur_ctx->begin?'{':'}',
	   ex[i]
	   );    
  }
}
/*------------------------------------------------------------*/
string ContextManager::getCurrentCtx(){
    CtxRIterator cx=ctx_list.rbegin();
    return (*cx)->ctx->getUniqueID() + ( (*cx)->begin?'{':'}');
  }

/*==================================================================*/


#endif // __GENERIC_CONTEXTS_HPP

