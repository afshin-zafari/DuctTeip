#ifndef FMM_TASKS_HPP
#define FMM_TASKS_HPP
#include <atomic>
#include <fstream>
#include "ductteip.hpp"
#include "fmm_types.hpp"
#include "fmm_data.hpp"

namespace FMM_3D{

  /*--------------------------------------------------------------------*/
  class DTTask : public IDuctteipTask  {
  public:
    int key,axs[4];
    std::string name;
    DTBase *args[4];
    DTTask *parent;
    std::atomic<size_t> child_count;
    virtual void run() = 0 ;
    virtual void finished(){}
    virtual void export_it(std::fstream&)=0;
  };
  /*--------------------------------------------------------------------*/
  class SGTask {
  public:
    double  work;
    int     key;
    DTTask *parent;
    virtual void run()=0;
  };
  /*========================================================================*/
  class FFLTask: public DTTask{
  public:
    GData *d1,*d2;
    /*----------------------------------------------------------------------*/
    FFLTask(GData &d1_,GData &d2_,DTTask *p){
      d1 = (GData *)&d1_;d2 =static_cast<GData *>(&d2_);
      key= DT_FFL;
      parent = p;
      parent->child_count ++;
    }
    FFLTask(GData &d1_,DTTask *p){
      d2 = static_cast<GData *>(&d1_);d1 = nullptr;
      key= DT_FFL;
      parent = p;
      parent ->child_count ++;
    }

    void finished(){
      if (--parent->child_count ==0)
	parent->finished();
    }
    void run();
    void export_it(fstream &f){}
  };
  
}//namespace FMM_3D
#endif
