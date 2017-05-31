#include "fmm_context.hpp"
#include "fmm_data.hpp"
namespace FMM_3D{
  long DTBase::last_handle=0;
  long DTBase::data_count=0;
  FMMContext *fmm_engine;
  void FMMContext::add_task(SGTask *t){
    counts[t->key]++;
    mu_tasks.push_back(t);
  }
  /*--------------------------------------------------------------------*/
  void FMMContext::add_task(DTTask *t)
  {
    counts[t->key]++;
    tasks.push_back(t);
  }
  /*--------------------------------------------------------------------*/
  void FMMContext::runKernels(DTTask *t)
  {
    switch(t->key)
      {
      case MVP:
	break;
      case Interpolation_Key:
	break;
      case Green_Translate:
	break;
      case Green_Interpolate:
	break;
      case Receiving_Key:
	break;
      default:
	break;
      }
  }

}//namespace FMM_3D

