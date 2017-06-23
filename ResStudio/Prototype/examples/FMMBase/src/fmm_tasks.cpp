#include "fmm_tasks.hpp"
#include "fmm_context.hpp"

namespace FMM_3D{
  int IterTask::last_iter_no = 0;
  GData *IterTask::d = nullptr;
  /*---------------------------------------------------------------*/
  void data_access(DataAccessList *dlist,GData *d,IData::AccessType rw){
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
    host = d2->getHost();
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
    double *A = d1->getContentAddress();
    double *B = d2->getContentAddress();
    int s=d1->getContentSize()/sizeof(ElementType);
    
    int lead = d1->lead_dim;
    cout << "Memory: " << A << " Size: " << s <<" Lead: " << lead << endl;
    d1->dump();
    s=d2->getContentSize()/sizeof(ElementType);
    lead = d2->lead_dim;
    cout << "Memory: " << B << " Size: " << s <<" Lead: " << lead << endl;
    d2->dump();
     
    for(int i=0;i<s;i++){
       B[i] +=A[i]; 
    }
    setFinished(true);
    finished();
  }
  /*---------------------------------------------------------------*/
  void IterTask::runKernel(){
  cout << "iter no = " << iter << endl;
    setFinished(true);
    finished();
  }
  /*---------------------------------------------------------------*/
  void IterTask::finished(){
    if ( last_iter_no <10) //norm_V_diff > 1e-6)
      fmm_engine->add_task ( new IterTask (1) );
    
  }
  /*---------------------------------------------------------------*/
  void IterTask::data_access_set(){
    parent_context = fmm_engine;
    setName("IterTask");
    d->setName("IterData");
    if(iter ==0)
      d->setRunTimeVersion("0.0",0);
    host = me;
    IDuctteipTask::key = key;
    DataAccessList *dlist = new DataAccessList;    
    data_access(dlist,d,(flag==0)?IData::READ:IData::WRITE);
    setDataAccessList(dlist);      
  }
  /*---------------------------------------------------------------*/
  /*---------------------------------------------------------------*/
}//namespace FMM_3D
