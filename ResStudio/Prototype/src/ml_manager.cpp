#include "ml_manager.hpp"

MLManager mlMngr;
SuperGlue<Options> SG;
map<ulong,sg_data_t*> g2sg_map;
map<ulong,dt_data_t*> g2dt_map;
/*--------------------------------------------------------------------*/
GData::GData(){
  p=NULL;level=NULL;child_cnt=0;sgData=NULL;dtData = NULL;
  mem = NULL;
  content = NULL;
  memMngr = NULL;
  type_size=sizeof(double);  
}
/*--------------------------------------------------------------------*/

GData::GData(int i,int j , int k){
  p=NULL;
  child_cnt=0;
  xn=i; // todo xn=j , yn=i
  yn=j;
  zn=k;
  type_size=sizeof(double);
  level=NULL;
  mem = NULL;
  content = NULL;
  memMngr = NULL;
  handle=mlMngr.getNewHandle();
  if ( j==0){
    dim_cnt =1;
  }
  if ( k==0 && j>0){
    dim_cnt =2;
  }
  if ( k>0 && j>0)
    dim_cnt=3;

}
/*--------------------------------------------------------------------*/
void GData::setAccess(int a){
  axs = a;
  for ( int i=0;i<child_cnt;i++)
    children[i].axs = a;
}
/*--------------------------------------------------------------------*/
void GData::setLevel(GenLevel*l){
  level=l;

  if(l==NULL )
    return;
  data_type=l->type;
  if ( data_type == GenLevel::LevelType::SG_TYPE){
    sgData = new sg_data_t;
  }
  if ( data_type == GenLevel::LevelType::DT_TYPE){
    dtData = new dt_data_t;
  }
  if ( l->getChild() == NULL) 
    return;
  setPartition(l->getChild()->p);
}
/*--------------------------------------------------------------------*/
GenHandle  *GData::getDataHandle(){
  return handle;
}
/*--------------------------------------------------------------------*/
void GData::getCoordination(int &y,int &x,int &z){
  if ( parent !=NULL){
    if ( parent->p!=NULL){
      x = child_idx/parent->p->y;
      y = child_idx%parent->p->y;  
    }
    else{
      LOG_INFO(LOG_MLEVEL,"Partition is not assigned.\n");
    }
  }
}
/*--------------------------------------------------------------------*/
void GData::allocateMemory(){
  if ( level ==NULL)
    return;
  if ( level->getMemoryAllocation()){
    long blk_size=(xn/p->x)*((yn?yn:1)/p->y)*((zn?zn:1)/p->z)*(type_size);
    LOG_INFO(LOG_MLEVEL,"L0:blk_size:%ld, child_cnt:%d\n",blk_size,getChildCount());
    memMngr = new MemoryManager(getChildCount(),blk_size);
  }
  else{
    if ( parent != NULL){
      if ( parent->memMngr != NULL){
	if ( mem ==NULL ) {
	  LOG_INFO(LOG_MLEVEL,"%p\n",parent->memMngr);
	  mem=parent->memMngr->getNewMemory();
	  if ( level->getChild() == NULL ) {
	    content = mem->getAddress();
	    LOG_INFO(LOG_MLEVEL,"L1:content:%p\n",content);	    
	  }
	}
      }
      else {
	if ( parent->mem != NULL){
	  long blk_size = xn*(yn?yn:1)*(zn?zn:1) * type_size;
	  content = parent->mem->getAddress()+child_idx*blk_size;
	  LOG_INFO(LOG_MLEVEL,"L2:idx:%d,blk-sz:%d,  content:%p\n",child_idx,blk_size,content);	    
	}
	if (parent->content !=NULL){
	  long blk_size = xn*(yn?yn:1)*(zn?zn:1) * type_size;
	  content = parent->content+child_idx*blk_size;
	  LOG_INFO(LOG_MLEVEL,"L3+:idx:%d,blk-sz:%d,  content:%p\n",child_idx,blk_size,content);	    
	}
      }
    }
  }
  
}
/*--------------------------------------------------------------------*/
byte *GData::getMemory(){
  if ( content != NULL)
    return content;
  if ( mem != NULL)
    return  mem->getAddress();
  return NULL;
}
/*--------------------------------------------------------------------*/
void GData::setElementValue(int row, int col,double val){
  byte *m = getMemory();
  if ( m==NULL) 
    return; 
  double *dm=(double*)(m+(col*yn+row)*type_size);
  *dm = val;
  
}
/*--------------------------------------------------------------------*/
void GData::setElementValue(int row, double val){
  setElementValue(row,0,val);
}
/*--------------------------------------------------------------------*/
double GData::getElementValue(int row , int col){
  byte *m = getMemory();
  if ( m==NULL) 
    return 0.0; 
  double *dm=(double*)(m+(col*yn+row)*type_size);
  return *dm;  
}
/*--------------------------------------------------------------------*/
int GData::getChildCount(){
  if ( level == NULL)
    return 0;
  GenLevel *child_level = level->getChild();
  if ( child_level ==NULL)
    return 0;
  GenPartition *pp=child_level->p;
  if ( pp==NULL)
    return 0;
  child_cnt = pp->x*pp->y*pp->z;
  return child_cnt;
}
/*--------------------------------------------------------------------*/
void GData::createChildren(){
  int col,row;
  if ( level == NULL)
    return;
  GenLevel *child_level = level->getChild();
  if ( child_level ==NULL)
    return;
  GenPartition *pp=child_level->p;
  if ( pp==NULL)
    return;
  child_cnt = pp->x*pp->y*pp->z;
  LOG_INFO(LOG_MLEVEL,"chld-cnt:%d\n",child_cnt);
  children= new GData[child_cnt];//todo children --> GData **
  for(int i=0;i<child_cnt;i++){
    children[i].parent=this;
    children[i].axs=axs;
    children[i].dim_cnt=dim_cnt;
    children[i].child_idx=i;
    children[i].xn = xn / pp->x;
    children[i].yn = yn / pp->y;    
    children[i].zn = zn / pp->z;
    col = i/pp->y;
    row = i%pp->y;
    children[i].xs = col * children[i].xn; 
    children[i].ys = row * children[i].yn; 
    children[i].handle=mlMngr.getNewHandle();
    children[i].setLevel(child_level);
    children[i].allocateMemory();
    children[i].createChildren();
  }
}
/*--------------------------------------------------------------------*/
GData &GData::operator()(int row, int col,int z){
  int ofs=row+col*p->y+z*p->x*p->y;
  if ( ofs< child_cnt)
    return children[ofs];
  LOG_ERROR("Index out of range.\n");
  return *this;
}

/*--------------------------------------------------------------------*/
void MLManager::submitTask(const char *func , Args * args,Axs &axs){
  for ( int i=0; i < args->args.size();i++){
    args->args[i]->axs = axs.axs[i];
  }
  /*
  LOG_INFO(LOG_MLEVEL,"axs-A:%d\n",args->args[0]->axs);
  LOG_INFO(LOG_MLEVEL,"axs-B:%d\n",args->args[1]->axs);
  LOG_INFO(LOG_MLEVEL,"args:%p\n",args);
  LOG_INFO(LOG_MLEVEL,"args-A:%p\n",args->args[0]);
  LOG_INFO(LOG_MLEVEL,"args-B:%p\n",args->args[1]);
  */
  GenTask *t=new GenTask(func,args,last_group);
  t->handle = new GenHandle(last_task_handle++);
  if (args!=NULL){
    if (args->args.size()>0) {
      if (args->args[0]!=NULL) {
	GenLevel *L=args->args[0]->getLevel();
	if (L!=NULL){
	  IScheduler *sch= L->scheduler;
	  if (sch!=NULL){
	    sch->submitTask(t);
	  }
	}
      }
    }
  }
  LOG_INFO(LOG_MLEVEL,"task arg0->mem:%p\n",t->args->args[0]->getMemory());
  tasks_list.push_back(t);
}
/*--------------------------------------------------------------------*/
void MLManager::addLevel(GenLevel *l){
  levels.push_back(l);
}
/*--------------------------------------------------------------------*/
GenLevel *MLManager::getActiveLevel(){
  if ( levels.size() >0 ) 
    return levels[0];
  return NULL;
}
/*--------------------------------------------------------------------*/
GenHandle *MLManager::getNewHandle(){
  GenHandle *n=new GenHandle(last_handle);
  if ( last_handle > MAX_HANDLES ){
    handles.resize(MAX_HANDLES*2);
  }
  handles[last_handle++]=n;
  return n;
}
/*--------------------------------------------------------------------*/
long  KeyGen(const char *s){//todo : unique based on 's' 
    return Taskified::LastFuncKey ++;
}
/*--------------------------------------------------------------------*/
void GenLevel::setParent(GenLevel *P){
  parent = P;
}
/*--------------------------------------------------------------------*/
void GenLevel::addChild(GenLevel *C){
  children.push_back(C);
  C->setParent(this);
}
/*--------------------------------------------------------------------*/
GenLevel * GenLevel::getChild(int i){
  if ( children.size()>0)
    return children[0];
  return NULL;
}
/*--------------------------------------------------------------------*/
void packArgs(Args *a){}
void packAxs(Axs &a){}
/*--------------------------------------------------------------------*/
void MLManager::submitNextLevelTasks(void *f, Args *args){
  //todo : make it thread safe
  last_group ++;
  KCALL(f,1);
  KCALL(f,2);
  KCALL(f,3);
}
/*--------------------------------------------------------------------*/
map<string ,void *> tasks_fps;
vector<GenTask*> tasks_list;
map<ulong,TaskHandle> gt2dt_map;
map<TaskHandle,GenTask *> dt2gt_map;

void MLManager::runTask(GenTask *t){
  void *fp = tasks_fps[t->fname];
  LOG_INFO(LOG_MLEVEL,"child-cnt:%d\n",t->args->args[0]->getChildrenCount());
  if (t->args->args[0]->getChildrenCount() ==0 ) {
    Args *args= t->args;
    if (args->args.size()>0) {
      if (args->args[0]!=NULL) {
	GenLevel *L=args->args[0]->getLevel();
	if (L!=NULL){
	  IScheduler *sch= L->scheduler;
	  if (sch!=NULL){
	    LOG_INFO(LOG_MLEVEL,"sch->runTask(t),t->arg[0].mem:%p\n",t->args->args[0]->getMemory());
	    sch->runTask(t);
	  }
	}
      }
    }
    /*
    LOG_INFO(LOG_MLEVEL,"task-name:%s\n",t->fname.c_str());
    LOG_INFO(LOG_MLEVEL,"A.xs:%d\n",t->args->args[0]->getElemSpanX_Start());
    LOG_INFO(LOG_MLEVEL,"A.xe:%d\n",t->args->args[0]->getElemSpanX_End());
    */
    return;
  }
  submitNextLevelTasks(fp,t->args);
}
/*----------------------------------------------------------------------------*/
int GData::getHost(){
  int type=getLevel()->type;
  if ( type != GenLevel::LevelType::DT_TYPE)
    return;
  GenHandle *gh=getDataHandle();
  IData *dt,*d;
  dt=g2dt_map[gh->handle];
  d = (*dt)(0,0);  
  return d->getHost();
}

/*----------------------------------------------------------------------------*/
void initData(DataArg A){
  int type=A.getLevel()->type;
  if ( type != GenLevel::LevelType::DT_TYPE)
    return;
  GenHandle *gh=A.getDataHandle();
  IData *dt,*d;
  dt=g2dt_map[gh->handle];
  d = (*dt)(0,0);  
  d->setRunTimeVersion("0.",0);
  dtEngine.putWorkForSingleDataReady(d);
  LOG_INFO(LOG_MLEVEL,"\n");
}

void GenAddDTTask( GenTask *t){
  IContext *ctx = (IContext *)new GenAlgorithm(t);
  
  glbCtx.incrementCounter(GlobalContext::TaskRead);  
  LOG_EVENTX(DuctteipLog::ReadTask,&dtEngine);
  
  IData *d,*dwrt,*dt;
  OneLevelData *dL1;
  glbCtx.incrementCounter(GlobalContext::TaskInsert);
  list<DataAccess *> *dlist = new list <DataAccess *>;
  DataAccess *daxs ;
  for ( int i=0; i < t->args->args.size(); i++){
    GenHandle *gh = t->args->args[i]->getDataHandle();      
    int row,col,depth;
    t->args->args[i]->getCoordination(row,col,depth);
    LOG_INFO(LOG_MLEVEL,"%d\n",gh->handle);
    if ( g2dt_map[gh->handle]==NULL || g2dt_map[gh->handle]==0){
      dL1=new OneLevelData("_",ctx);
      dL1->setBlockIdx(row,col);
      g2dt_map[gh->handle]=(IData *)dL1;
    }
    else{
      LOG_INFO(LOG_MLEVEL,"\n");
    }
    dt=g2dt_map[gh->handle];
    d = (*dt)(0,0);
    d->setBlockIdx(row,col);
    d->setRunTimeVersion("-1",-1);
    d->resetVersion();
    int r,c;
    d->getBlockIdx(r,c);
    LOG_INFO(LOG_MLEVEL,"d:%s, host:%d, blkidx(%d,%d)\n",d->getName().c_str(),d->getHost(),r,c);
    daxs = new DataAccess;
    daxs->data = d;
    if ( t->args->args[i]->axs == In){
      daxs->type = IData::READ;
      daxs->required_version = d->getWriteVersion();
      d->incrementVersion(IData::READ);
      LOG_INFO(LOG_MLEVEL,"READ  A(%d,%d)@%d-ver:%s\n",row,col,d->getHost(),
	       d->getRunTimeVersion(IData::READ).dumpString().c_str());
    }
    else{
      dwrt = d;
      daxs->type = IData::WRITE;
      daxs->required_version = d->getReadVersion();
      d->incrementVersion(IData::WRITE);
      LOG_INFO(LOG_MLEVEL,"WRITE B(%d,%d)@%d-ver:%s\n",row,col,d->getHost(),
	       d->getRunTimeVersion(IData::WRITE).dumpString().c_str());
    }
    daxs->required_version.setContext( glbCtx.getLevelString() );
    d->getWriteVersion().setContext( glbCtx.getLevelString() );
    d->getReadVersion().setContext( glbCtx.getLevelString() );
    dlist->push_back(daxs);

  }
  if ( dwrt->getHost()==me){
    LOG_INFO(LOG_MLEVEL,"task added.\n");
    TaskHandle task_handle =dtEngine.addTask(ctx,t->fname,KeyGen(t->fname.c_str()),dwrt->getHost(),dlist);  
    gt2dt_map[t->handle->handle]=task_handle;
    dt2gt_map[task_handle]=t;
  }
}

