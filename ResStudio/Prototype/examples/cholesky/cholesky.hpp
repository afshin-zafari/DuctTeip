#ifndef __CHOLESKY_HPP__
#define __CHOLESKY_HPP__

#include "ductteip.hpp"
#include "math.h"
#include "cblas.h"
#include "lapacke.h"
#define dgemm cblas_dgemm
#define dpotrf LAPACK_dpotrf
#define dsyrk cblas_dsyrk
#define dtrsm cblas_dtrsm
#ifdef ACML
#include <acml.h>
#endif
#include "potrf.hpp"
#include "syrk.hpp"
#include "trsm.hpp"
#include "gemm.hpp"

class Cholesky: public Algorithm{
private:
  // The input/output data of Cholesky factorization
  Data  *M;                          /*@\label{line:memberdata}@*/
  enum KernelKeys {POTRF,TRSM,GEMM,SYRK};       /*@\label{line:taskkeys}@*/
public:
  
  Cholesky( );      /*@\label{line:ctor}@*/

  // The taskified version of Cholesky factorization
  void taskified();

  // Called by the framework, when a task is ready to run
  void runKernels(DuctTeip_Task *task );

  // Kernels of the tasks 
  void POTRF_kernel(DuctTeip_Task *dt_task);
  void  TRSM_kernel(DuctTeip_Task *dt_task);
  void  SYRK_kernel(DuctTeip_Task *dt_task);
  void  GEMM_kernel(DuctTeip_Task *dt_task);

  string getTaskName(unsigned long key);
  void  taskFinished(DuctTeip_Task *task, TimeUnit dur);
  void  checkCorrectness();
  void  populateMatrice();
  void  dumpAllData();
  ~Cholesky();
};
#endif //__CHOLESKY_HPP__
