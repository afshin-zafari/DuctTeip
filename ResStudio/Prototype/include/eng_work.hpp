struct DuctTeipWork{
public:
  enum WorkTag{
    TaskWork,
    ListenerWork,
    DataWork
  };
  enum WorkEvent{
    Ready,
    Sent,
    Finished,
    DataSent,
    DataReceived,
    DataUpgraded,
    DataReady,
    Received,
    Added
  };
  enum TaskState{
    Initialized,
    WaitForData,
    Running
  };
  enum WorkItem{
    CheckTaskForData,
    CheckTaskForRun,
    SendTask,
    CheckListenerForData,
    UpgradeData,
    CheckAfterDataUpgraded,
    SendListenerData
  };
  IDuctteipTask *task;
  IData *data;
  IListener *listener;
  int state,tag,event,host,item;
  DuctTeipWork &operator =(DuctTeipWork *_work);
  void dump();
};
