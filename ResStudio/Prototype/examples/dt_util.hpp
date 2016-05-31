#include "ductteip.hpp"
/*----------------------------------------------------------------------------*/
struct SyncTask : public Task<Options, -1> {
  IDuctteipTask *dt_task ;
  string log_name;
  SyncTask(IDuctteipTask *task_);
  SyncTask(Handle<Options> **h,int M,int N,IDuctteipTask *task_);
  void run(TaskExecutor<Options> &te) ;
  ~SyncTask();
  string get_name();
};
/*----------------------------------------------------------------------------*/
class ElementType_Data {
private :
public:
  int N,M;
  double *memory;
  ElementType_Data(Task<Options,-1> *t,int arg,int &m, int &n){
    arg++;
    N=n = t->getAccess(arg).getHandle()->block->X_E();
    M=m = t->getAccess(arg).getHandle()->block->Y_E();
    memory = t->getAccess(arg).getHandle()->block->getBaseMemory();
  }
  double operator()(int i,int j){
    return memory[j*M+i];
  }
  double &operator[](int i){
    return memory[i];
  }
  const double &operator[](int i)const {
    return memory[i];
  }
  
};

/*----------------------------------------------------------------------------*/
//# define Mat(mat,i,j) mat[j*N+i]
#define SG_DEFINE_TASK_ARGS1(name,kernel)

  //  template <int N>
  class TaskDTLevel: public Task<Options, -1>{
  private:
    IDuctteipTask *dt_task ;
    string log_name;
  public:
    TaskDTLevel(IDuctteipTask *task_,Handle<Options> &h1): dt_task(task_) {
      registerAccess(ReadWriteAdd::read, *dt_task->getSyncHandle());
      registerAccess(ReadWriteAdd::write, h1);
      ostringstream os;
      os << setfill('0') << setw(4) <<dt_task->getHandle() << "name" ;
      log_name = os.str();
    }
    void run(){}
    string get_name(){return log_name;}
  };
/*----------------------------------------------------------------------------*/

class SGTask4DTKernel:public Task<Options,-1>{

protected: 
    IDuctteipTask *dt_task ;
    string log_name;
public:
  SGTask4DTKernel(IDuctteipTask *task_):dt_task(task_){
      registerAccess(ReadWriteAdd::read, *dt_task->getSyncHandle());
  }
  string get_name(){ return log_name;}
  
};
/*----------------------------------------------------------------------------*/
