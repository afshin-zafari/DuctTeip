#ifdef ACML
#include <acml.h>
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
    registerAccess(ReadWriteAdd::write, A);/*@\label{line:sgreg}@*/
  }
  //---------------------------------------------------------
  // This is called by the SuperGlue framework, 
  //   when the task is ready to run.
  void runKernel() {
    // Get the first argument of the task 
    LastLevel_Data a = get_argument(0);/*@\label{line:lld}@*/

    // Get the memory address and its size, 
     LOG_INFO(LOG_DATA,"\n");
    double *mem = a.get_memory();/*@\label{line:lldmem}@*/
    int info, N = a.get_rows_count();/*@\label{line:lldsz}@*/
    char side[2];
    side[0]='L';side[1]=0;
    
    assert(mem);
    dpotrf(side,&N,mem,&N,&info);
  }
};
