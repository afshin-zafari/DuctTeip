#ifndef FMM_CONTEXT_HPP
#define FMM_CONTEXT_HPP
#include <list>
#include "fmm_types.hpp"
#include "fmm_tasks.hpp"
#include "ductteip.hpp"
namespace FMM_3D{
  /*--------------------------------------------------------------------*/
  class FMMContext : public IContext 
  {
  private:
    list<DTTask*> tasks;
    list<SGTask*> mu_tasks;
  public:
    long counts[NUM_TASK_KEYS+1];
    ~FMMContext(){
      tasks.clear();
      mu_tasks.clear();
    }
    FMMContext():IContext("FMM_3D"){
      for ( int i=0;i<NUM_TASK_KEYS+1;i++)
	counts[i]=0;
    }
    /*--------------------------------------------------------------------*/
    void add_task(SGTask *t);
    void add_task(DTTask *t);
    void export_tasks(string);
    void runKernels(DTTask *t);
    string getTaskName(unsigned long ){return ""; }
    void taskFinished(IDuctteipTask *,TimeUnit){}
  };
  /*--------------------------------------------------------------------*/
  extern FMMContext *fmm_engine;
  /*--------------------------------------------------------------------*/
}//namespace FMM_3D
#endif
