
#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <string>
#include <cstdio>

#include <vector>
#include <list>

#include "config.hpp"
#include "procgrid.hpp"
#include "hostpolicy.hpp"
#include "data.hpp"
#include "glb_context.hpp"
#include "engine.hpp"
#include "dt_log.hpp"
#include "data_basic.hpp"

using namespace std;
extern engine dtEngine;

extern int me,version,simulation;

class IContext;
class IData;
/*===============================  Context Class =======================================*/
class IContext
{
protected:
  string           name;
  IContext        *parent;
  list<IContext*>  children;
  list<IData*>     inputData,
    outputData,
    in_outData;
  ProcessGrid     *PG;
  ContextHandle   *my_context_handle;
  Config *cfg;
public:
  IContext(string _name);
  ~IContext();
  virtual void runKernels(IDuctteipTask *task) = 0 ; 
  virtual string getTaskName(unsigned long) =0;

  string getName()    {return name;}
  string getFullName(){return getName();}
  void   print_name       ( const char *s="") {cout << s << name << endl;}
  void   setContextHandle ( ContextHandle *c) {my_context_handle = c;}
  ContextHandle *getContextHandle ()                  { return my_context_handle;}

  IData *getDataFromList(list<IData* > dlist,int index);
  IData *getOutputData  (int index=0);    
  IData *getInputData   (int index=0);    
  IData *getInOutData   (int index=0);    
  IData *getDataByHandle(list<IData *> dlist,DataHandle *dh);
  IData *getDataByHandle(DataHandle *dh ) ;
  void setParent ( IContext *_p ) ;
  void traverse();
  void addInputData(IData *_d);
  void addOutputData(IData *_d);
  void addInOutData(IData *_d);
  void resetVersions();
  void dumpDataVersions(ContextHeader *hdr=NULL);
  void testHandles(list<IData*> dlist);
  void testHandles();
  DataHandle * createDataHandle ( ) ;
};
/*===================================================================================*/
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
typedef Context Algorithm;
/*===================================================================================*/
#endif