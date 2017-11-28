#ifdef USE_MKL
#include <mkl.h>
#endif
// A SuperGlue task that is submitted 
//   in the kernel of a DuctTeip task
class PotrfTask : public SuperGlueTaskBase {
public:
  // At construction time, 
  //    gets the parent DuctTeip task 
  //    and the input/output SuperGlue data 
  PotrfTask( DuctTeip_Task *task_,Handle<Options> &A):
    SuperGlueTaskBase(task_) {
    //register_access(ReadWriteAdd::read,*task_->getSyncHandle());
    //registerAccess(ReadWriteAdd::write, A);/*@\label{line:sgreg}@*/
    LOG_INFO(LOG_TASKS,"CTOR sgPotrf for parent:%s.\n",task_->getName().c_str());
    name.assign("potrf");
  }
  string get_name(){return string( "potrf");}
  //---------------------------------------------------------
  // This is called by the SuperGlue framework, 
  //   when the task is ready to run.
  void runKernel() {
    // Get the first argument of the task 
    LastLevel_Data a = get_argument(0);/*@\label{line:lld}@*/

    // Get the memory address and its size, 
     LOG_INFO(LOG_DATA,"runKernel of sgPotrf\n");
    double *mem = a.get_memory();/*@\label{line:lldmem}@*/
    int info, N = a.get_rows_count();/*@\label{line:lldsz}@*/
    char side[2];
    side[0]='L';side[1]=0;
    
    assert(mem);
#ifdef USE_OPENBLAS
    dpotrf(side,&N,mem,&N,&info); 
#endif
#ifdef USE_MKL
    int nb = N;
    dpotrf("L", &nb, mem, &nb, &info);
#endif
  }
};
