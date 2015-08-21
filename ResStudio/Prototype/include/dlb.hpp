#ifndef __DLB_HPP_
#define  __DLB_HPP_
#include "dt_task.hpp"
#include "mailbox.hpp"
#include "data.hpp"
#include "engine.hpp"
#include "glb_context.hpp"

class DLB{
private:
  friend class engine;
  struct DLB_Statistics{
    unsigned long tot_try,
      tot_failure,
      tot_tick,
      max_loc_fail,export_task,export_data,import_task,import_data,max_para,max_para2;
    ClockTimeUnit tot_cost,
      tot_silent;      
  };
  DLB_Statistics dlb_profile;
  int dlb_state,dlb_prev_state,dlb_substate,dlb_stage,dlb_node;
  unsigned long dlb_failure,dlb_glb_failure;
  TimeUnit dlb_silent_start;
  enum DLB_STATE{
    DLB_IDLE,
    DLB_BUSY,
    TASK_EXPORTED,
    TASK_IMPORTED,
    DLB_STATE_NONE
  };
  enum DLB_SUBSTATE{
    ACCEPTED,
    EARLY_ACCEPT
  };
  enum DLB_STAGE{
    DLB_FINDING_IDLE,
    DLB_FINDING_BUSY,
    DLB_SILENT,
    DLB_NONE
  };
  enum Limits{
    SILENT_PERIOD=100,
    FAILURE_MAX=5
  };
  int find[2][2],decline[2][2],accept[2][2];
  enum Indices{
    IDX_IDLE=0,IDX_BUSY=1,
    IDX_ME=0,IDX_OTHERS=1
  };
#define TIME_DLB 
  /*---------------------------------------------------------------*/
  bool passedSilentDuration();
  void initDLB();
  void updateDLBProfile();
  void restartDLB(int s=-1);    
  void doDLB(int st);
  void goToSilent();
  void dumpDLB();
  int getDLBStatus();

  void findBusyNode();
  void findIdleNode();
  void received_FIND_IDLE(int p);
  void receivedImportRequest(int p); 
  void received_FIND_BUSY(int p ) ;
  void receivedExportRequest(int p);
  void received_DECLINE(int p);
  void receivedDeclineMigration(int p);
  IDuctteipTask *selectTaskForExport();
  void exportTasks(int p);
  void importData(MailBoxEvent *event);
  void receiveTaskOutData(MailBoxEvent *event);
  void receivedMigrateTask(MailBoxEvent *event);
  void received_ACCEPTED(int p);
  void declineImportTasks(int p);    
  void declineExportTasks(int p);    
  void exportTask(IDuctteipTask* task,int destination);
  IData * importedData(MailBoxEvent *event,MemoryItem *mi);
  void    importedTask(MailBoxEvent *event);
  bool isFinishedEarlier(IDuctteipTask *t);
  /*---------------------------------------------------------------*/
  void acceptImportTasks(int p);    
  /*---------------------------------------------------------------*/
private:
  struct Failure{
    int node,request;
    ulong timestamp;
    Failure(int p,ulong t,int r):node(p),timestamp(t),request(r){}
  };
  vector<Failure *> dlb_fail_list;
public:
  bool isInList(int p,int req);
  int  getRandomNodeEx(int r);
  int  getRandomNodeOld(int exclude =-1);
  void checkImportedTasks();
  void checkExportedTasks();
  long getActiveTasksCount();
/*---------------------------------------------------------------------------------*/

};
extern DLB dlb;
#endif // __DLB_HPP_
