#include "fmm_tasks.hpp"
#include "fmm_context.hpp"

namespace FMM_3D{
  /*---------------------------------------------------------------*/
  void FFLTask::data_access(DataAccessList *dlist,GData *d,IData::AccessType rw){
    DataAccess *dx1 = new DataAccess;
    dx1->data = d;
    if ( rw == Data::READ){
      dx1->required_version = d->getWriteVersion();
      dx1->type = IData::READ;
    }
    else{
      dx1->required_version = d->getReadVersion();
      dx1->type = IData::WRITE;
    }
    dx1->required_version.setContext( glbCtx.getLevelString() );
    d->getWriteVersion().setContext( glbCtx.getLevelString() );
    d->getReadVersion().setContext( glbCtx.getLevelString() );
    dlist->push_back(dx1);
    d->incrementVersion(rw);
  }
  /*---------------------------------------------------------------*/
  void FFLTask::data_access_set(){
    host = me;
    parent_context = fmm_engine;
    setName("FFLTask");
    IDuctteipTask::key = key;
    DataAccessList *dlist = new DataAccessList;
    if (d1)
      data_access(dlist,d1,IData::READ );
    data_access(dlist,d2,IData::WRITE);
    setDataAccessList(dlist);      
  }
  /*---------------------------------------------------------------*/
  void FFLTask::runKernel(){
    std::cout << "FFLTask::run() called for " << d2->getName() << endl;
    setFinished(true);
    finished();
  }
  /*---------------------------------------------------------------*/
  /*---------------------------------------------------------------*/
  /*---------------------------------------------------------------*/
}//namespace FMM_3D
