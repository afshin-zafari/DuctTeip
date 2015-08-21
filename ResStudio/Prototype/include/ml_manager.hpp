#ifndef __ML_MANAGER_HPP__
#define __ML_MANAGER_HPP__
#include "basic.hpp"
#include "data_basic.hpp"
#include "data.hpp"
#include "partition.hpp"
#include "glb_context.hpp"
#include "engine.hpp"
#include "context.hpp"
#include <vector>
#include <functional>

const int In=1;
const int Out=2;
const int InOut=3;

class GenTask;
class IScheduler{
public:
  virtual void submitTask(GenTask *)=0;
  virtual void runTask(GenTask *)=0;
  virtual void finishedTaskGroup(ulong g)=0;
};
/*====================================================================*/
struct GenPartition{
  int x,y,z;
  GenPartition(){}
  GenPartition(int xx,int yy=1,int zz=1){
    x=xx;
    y=yy;
    z=zz;
  }
};
/*====================================================================*/
struct GenLevel{
  GenPartition *p;
  GenLevel *parent;

  vector<GenLevel *> children;
  int type;
  IScheduler *scheduler;
  enum LevelType{SG_TYPE,DT_TYPE,BLAS_TYPE,CU_TYPE};
  GenLevel(GenPartition *P){p=P;scheduler=NULL;}
  void setParent(GenLevel *);
  void addChild (GenLevel *);
  void setType(LevelType t){type = t;}
  GenLevel* getChild(int i=0);
};
typedef Handle<Options> sg_data_t;
typedef IData dt_data_t;
struct GenHandle{
  ulong handle;
  GenHandle(ulong h):handle(h){}
};
/*====================================================================*/
class GData{
private:
  GenPartition *p;
  GenLevel *level;
  GData *children,*parent;
  int    dim_cnt,xn,yn,zn,child_cnt,child_idx;
  int    xs,xe,ys,ye,zs,ze,data_type;
  dt_data_t  *dtData;
  sg_data_t  *sgData;
  GenHandle *handle;

public:
  int axs;
  void setAccess(int a);
  int getElemCountX(){return xn;}
  int getElemCountY(){return yn;}
  int getElemCountZ(){return zn;}
  int getPartCountX(){if(!p) return 0;return p->x;}
  int getPartCountY(){if(!p) return 0;return p->y;}
  int getPartCountZ(){if(!p) return 0;return p->z;}
  int getElemSpanX_Start(){return xs;}
  int getElemSpanX_End(){return xs+xn;}
  int getElemSpanY_Start(){return ys;}
  int getElemSpanY_End(){return ys+yn;}
  int getChildrenCount(){return child_cnt;}
  void getCoordination(int &y, int &x, int &z);
  void createChildren();
  void setLevel(GenLevel*l);
  GenLevel* getLevel(){return level;}
  GData(){p=NULL;level=NULL;child_cnt=0;sgData=NULL;dtData = NULL;}
  GData(int i,int j=0, int k=0);
  void setPartition(GenPartition *P){p=P;};
  GData &operator()(int i, int j=0,int k=0);
  GenHandle  *getDataHandle();
  int getDataType(){return data_type;}
};
typedef GData GenData;
typedef GenData &DataArg;
/*====================================================================*/
/*====================================================================*/
struct Args{
  vector<GData*> args;
  //todo: access to be member of Args. -> thread safety
  Args(){args.reserve(5);}
  void addArg(GData *a){
    args.push_back(a);
  }
};
/*====================================================================*/
struct Axs{
  vector<int> axs;
  Axs(){axs.reserve(5);}
  void addAxs(int a){
    axs.push_back(a);
  }
};
/*====================================================================*/
struct GenTask{
  string fname;
  Args *args;
  ulong group;
  GenHandle *handle;
  GenTask(const char *s,Args *A,ulong g=0):fname(s),args(A),group(g){}
};
extern vector<GenTask*> tasks_list;

/*====================================================================*/
static const int  MAX_HANDLES =60000;
class MLManager{
private:
  vector<GenLevel*> levels;
  vector<GenHandle*> handles;
public:  
  ulong last_handle,last_group,last_task_handle;
  MLManager(){last_task_handle=last_handle=0;handles.reserve(MAX_HANDLES);}
  void submitTask ( const char *, Args *,Axs &);
  void  addLevel(GenLevel *l);
  GenLevel *getActiveLevel();
  GenHandle *getNewHandle();
  void runTask(GenTask *);
  void finishedTaskGroup(ulong g);
  void submitNextLevelTasks(void *f,Args *);
};

extern MLManager mlMngr;
/*============================================================================*/
/*                            SG WRAPPER                                      */
/*============================================================================*/
extern map<ulong,sg_data_t*> g2sg_map;
extern map<ulong,dt_data_t*> g2dt_map;
class SGGenTask:public Task<Options>{
private:
  GenTask *gt;int k;
public:  
  SGGenTask(GenTask *t):gt(t){
    Args *a=t->args;
    for(int i=0;i< a->args.size(); i++){
      GenHandle *gh = a->args[i]->getDataHandle();
      if (g2sg_map[gh->handle]==NULL)
	g2sg_map[gh->handle]=new sg_data_t;
      sg_data_t *h = g2sg_map[gh->handle];
      if ( a->args[i]->axs==In)
	registerAccess(ReadWriteAdd::read,*h);
      else
	registerAccess(ReadWriteAdd::write,*h);
    }
  }
  void run(TaskExecutor<Options> &te){
    //LOG_INFO(LOG_MLEVEL,"%s\n",gt->fname.c_str());
    mlMngr.runTask(gt);
  }  
  string get_name(){return gt->fname;}
};
extern SuperGlue<Options> SG;
class SGWrapper:public IScheduler{
public:
  void submitTask(GenTask*t){
    //LOG_INFO(LOG_MLEVEL,"%s\n",t->fname.c_str());
    SG.submit( new SGGenTask(t));
  }
  void runTask(GenTask *t){
    //LOG_INFO(LOG_MLEVEL,"%s\n",t->fname.c_str());
  }
  void finishedTaskGroup(ulong g){}
  SGWrapper(){}
};

long  KeyGen(const char *s);
void submitNextLevelTasks ( void*,Args*);
void GenAddDTTask (GenTask *t);

/*============================================================================*/
/*                            DT WRAPPER                                      */
/*============================================================================*/
extern map<ulong,TaskHandle> gt2dt_map;
extern map<TaskHandle,GenTask *> dt2gt_map;

class OneLevelData:public IData{
private:
public:
  OneLevelData(string name,IContext *ctx):IData(name,1,1,ctx){
    IData::parent_data = NULL;
    if ( getParent())
      setDataHandle( getParent()->createDataHandle());
    setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
    setLocalNumBlocks(1,1);  
    setPartition(1,1);     

    //    LOG_INFO(LOG_MLEVEL,"parent data:%p hpData:%p.\n",parent_data,hpData);
  }
};
class GenAlgorithm : public Algorithm{
private:
  GenTask *gt;
public :
  GenAlgorithm(GenTask *t):gt(t){
  }
  void taskFinished(IDuctteipTask *task, TimeUnit dur){
    LOG_INFO(LOG_MLEVEL,"%s, dur:%ld\n",task->getName().c_str(),dur);
  }
  void runKernels(IDuctteipTask *task) {
    LOG_INFO(LOG_MLEVEL,"%s\n",task->getName().c_str());
    GenTask *t=dt2gt_map[task->getHandle()];
    mlMngr.runTask(t);

  }  
  string getTaskName(unsigned long) {return gt->fname;}

};

class DTWrapper:public IScheduler{
public:
  void submitTask(GenTask*t){
    LOG_INFO(LOG_MLEVEL,"%s\n",t->fname.c_str());
    GenAddDTTask(t);
  }
  void runTask(GenTask *t){
    LOG_INFO(LOG_MLEVEL,"%s\n",t->fname.c_str());
    TaskHandle th = gt2dt_map[t->handle->handle];
    IDuctteipTask *tt=dtEngine.getTaskByHandle(th);
    tt->setFinished(true);
  }
  void finishedTaskGroup(ulong g){}

  DTWrapper(){}
};

/*=====================================================================*/
/*                        MACROS                                       */
/*=====================================================================*/

/*---------------------------------------------------------------------*/
namespace Taskified{
  static long LastFuncKey=0;
}
long KeyGen(const char *s);
void submitNextLevelTasks ( void*,Args*);
void GenAddDTTask (GenTask *t);
void initData(DataArg);


/*====================================================================*/

void packArgs(Args *a);
void packAxs (Axs  &a);
template <typename T, typename... P>
void packArgs(Args *a,T* arg, P... args){    
    a->addArg(arg);
    packArgs(a,args...);
}
template <typename T, typename... P>
void packAxs(Axs &a,T axs, P... args){    
    a.addAxs(axs);
    packAxs(a,args...);
}

extern map<string ,void *> tasks_fps;
struct tempKernel{
  tempKernel(const char *fn,void *fp){
    tasks_fps[string(fn)]=fp;
  }
};
#define DefKernel(f) tempKernel kernels_##f ( #f, (void *)f);
#define GetKernel(f) tasks_fps[#f]
#define taskify(function,axs...)			\
  DefKernel(function);					\
  namespace Taskified{					\
    template <typename... P>				\
    void function(P&... args){				\
      Args *a=new Args;						\
      Axs b;						\
      packArgs(a,&args...);				\
      packAxs (b,##axs);				\
      mlMngr.submitTask(#function,a,b);			\
    }							\
}


typedef void (*PF1)(DataArg);
typedef void (*PF2)(DataArg,DataArg);
typedef void (*PF3)(DataArg,DataArg,DataArg);
#define ARGS(k) *(args->args[k])
#define BIND1(f) std::bind((PF1)f,ARGS(0))
#define BIND2(f) std::bind((PF2)f,ARGS(0),ARGS(1))
#define BIND3(f) std::bind((PF3)f,ARGS(0),ARGS(1),ARGS(2))
#define KCALL(f,a)				\
  if ( args->args.size() ==a){			\
    auto kernel = BIND##a(f);			\
    kernel();					\
  }						\



#define MULTI_LEVEL 1
#if MULTI_LEVEL==1
#define CALL Taskified::
#else
#define CALL
#endif

#endif //__ML_MANAGER_HPP__
