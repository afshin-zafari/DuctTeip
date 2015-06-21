#include "ductteip.hpp"
/*----------------------------------------------------------------------------*/
struct SyncTask : public Task<Options, -1> {
  IDuctteipTask *dt_task ;
  string log_name;
  SyncTask(IDuctteipTask *task_):dt_task(task_) {
    PRINT_IF(0)("sync task ctor,task:%s\n",dt_task->getName().c_str());
        registerAccess(ReadWriteAdd::write, *dt_task->getSyncHandle());
	ostringstream os;
	os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_sync ";
	log_name = os.str();
    }
  SyncTask(Handle<Options> **h,int M,int N,IDuctteipTask *task_):dt_task(task_) {
    for(int i =0;i<M;i++)
      for (int j=0;j<N;j++)
        registerAccess(ReadWriteAdd::write, h[i][j]);
    printf("sync on all handles:%d,%d for:%s\n",M,N,dt_task->getName().c_str());
    }
    void run() {  }
    ~SyncTask(){
      PRINT_IF(KERNEL_FLAG)("[%ld] : sg_sync task for:%s starts running.\n",pthread_self(),
			    dt_task->getName().c_str());
      dt_task->setFinished(true);
    }
  string get_name(){ return log_name;}
  //string getName(){ return log_name;}
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
  void f(IDuctteipTask *t,Handle<Options> &h1){
    TaskDTLevel *PotrfTaskDT=new TaskDTLevel (t,h1);
  }
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

  class SuperGlue_Data {
  private:
    Handle<Options> **hM;
  public:
    SuperGlue_Data(IData *d, int &m,int &n){
      hM=d->createSuperGlueHandles();
      m = d->getYLocalNumBlocks();
      n = d->getXLocalNumBlocks();
    }
    Handle<Options> &operator()(int i,int j ){
      return hM[i][j];
    }
  };
