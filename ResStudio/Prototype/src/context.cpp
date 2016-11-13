#include "context.hpp"
int me;

/*===============================  Context Class =======================================*/
/*--------------------------------------------------------------------------------------*/
IContext::IContext(string _name) 
{
    name=_name;
    parent =NULL;
    setContextHandle ( glbCtx.createContextHandle() ) ;
    glbCtx.addContext( this ) ;
}
/*--------------------------------------------------------------------------------------*/
IContext::~IContext()
{
    list<IData*>::iterator it ;
    list<IContext*>::iterator ctx_it ; 
    for ( ctx_it =children.begin(); ctx_it != children.end(); ++ ctx_it )
    {
        delete (*ctx_it);
    }
    inputData.clear();
    outputData.clear();
    children.clear();
}
/*--------------------------------------------------------------------------------------*/
IData *IContext::getDataFromList(list<IData* > dlist,uint index)
{
    list<IData *>:: iterator it;
    if ( index >= dlist.size() )
        return NULL;
    it=dlist.begin();
    advance(it,index);
    return (*it);
}
/*--------------------------------------------------------------------------------------*/
IData *IContext::getOutputData  (int index)
{
    return getDataFromList ( outputData , index);
}
/*--------------------------------------------------------------------------------------*/
IData *IContext::getInputData   (int index)
{
    return getDataFromList ( inputData , index);
}
/*--------------------------------------------------------------------------------------*/
IData *IContext::getInOutData   (int index)
{
    return getDataFromList ( in_outData , index);
}
/*--------------------------------------------------------------------------------------*/
IData *IContext::getDataByHandle(list<IData *> dlist,DataHandle *dh)
{
    list<IData*> :: iterator it;
    IData* data;
    for (it = dlist.begin(); it != dlist.end(); ++it)
    {
        data = (*it)->getDataByHandle(dh);
        if ( data )
        {
            if ( data ->getName().size() != 0 )
                return data;
        }
    }
    return NULL;
}
/*--------------------------------------------------------------------------------------*/
IData *IContext::getDataByHandle(DataHandle *dh )
{
    IData *data=NULL;
    data = getDataByHandle ( inputData,dh);
    if ( data )
        if ( data ->getName().size() != 0 )
            return data;
    data = getDataByHandle ( outputData,dh);
    if ( data )
        if ( data ->getName().size() != 0 )
            return data;
    data = getDataByHandle ( in_outData,dh);
    if ( data )
        if ( data ->getName().size() != 0 )
            return data;
    return data;
}

/*--------------------------------------------------------------------------------------*/
void IContext::setParent ( IContext *_p )
{
    parent = _p;
    parent->children.push_back(this);
}
/*--------------------------------------------------------------------------------------*/
void IContext::addInputData(IData *_d)
{
    inputData.push_back(_d);
}
/*--------------------------------------------------------------------------------------*/
void IContext::addOutputData(IData *_d)
{
    outputData.push_back(_d);
}
/*--------------------------------------------------------------------------------------*/
void IContext::addInOutData(IData *_d)
{
    in_outData.push_back(_d);
    LOG_INFO(LOG_MULTI_THREAD,"add in-out data ptr:%p, len:%ld\n",_d,in_outData.size());
}
/*--------------------------------------------------------------------------------------*/
DataHandle * IContext::createDataHandle ( )
{
    DataHandle *d =glbCtx.createDataHandle () ;
    d->context_handle = *my_context_handle ;
    if(0)printf("@data se dh:%ld\n",d->data_handle);
    return d;
}
/*----------------------------------------------------------------------------*/
void IContext::addTask(ulong key,IData* d1,IData *d2,IData *d3){
  string s= getTaskName(key);
  //printf("===============\n addTask:s:%s, Key:%ld, d1:%p, d2:%p , d3:%p.\n",	 s.c_str(),key,d1,d2,d3);
  if (d2==NULL && d3 == NULL){
    AddTask(this,s,key,NULL,NULL,d1);
    return;
  }
  if (d3 == NULL){
    AddTask(this,s,key,d1,NULL,d2);
    return;
  }
  AddTask(this,s,key,d1,d2,d3);
}
/*----------------------------------------------------------------------------*/
void AddTask ( IContext *ctx,
               string s,
               unsigned long key,
               IData *d1,
               IData *d2,
               IData *d3)
{
    ContextHeader *c=NULL;
    if ( !glbCtx.getTaskReadHostPolicy()->isAllowed(ctx,c) )
    {
        printf("ctx read task disallowed.\n");
        return;
    }

    glbCtx.incrementCounter(GlobalContext::TaskRead);

    DataRange * dr = new DataRange;
    dr->d = d3->getParentData();
    Coordinate b = d3->getBlockIdx();
    dr->row_from = dr->row_to = b.by;
    dr->col_from = dr->col_to = b.bx;
    c = new ContextHeader ;
    c->addDataRange(IData::WRITE,dr);
    LOG_EVENTX(DuctteipLog::ReadTask,&dtEngine);



    if ( glbCtx.getTaskAddHostPolicy()->isAllowed(ctx,c) )
    {
        glbCtx.incrementCounter(GlobalContext::TaskInsert);
        if ( !d3->isOwnedBy(me) )
        {
            glbCtx.incrementCounter(GlobalContext::CommCost);
        }
        list<DataAccess *> *dlist = new list <DataAccess *>;
        DataAccess *daxs ;
        if ( d1 != NULL )
        {
            daxs = new DataAccess;
            daxs->data = d1;
            daxs->required_version = d1->getWriteVersion();
            daxs->required_version.setContext( glbCtx.getLevelString() );
            d1->getWriteVersion().setContext( glbCtx.getLevelString() );
            d1->getReadVersion().setContext( glbCtx.getLevelString() );
            daxs->type = IData::READ;
            dlist->push_back(daxs);
        }
        if ( d2 != NULL )
        {
            daxs = new DataAccess;
            daxs->data = d2;
            daxs->required_version = d2->getWriteVersion();
            daxs->required_version.setContext( glbCtx.getLevelString() );
            d2->getWriteVersion().setContext( glbCtx.getLevelString() );
            d2->getReadVersion().setContext( glbCtx.getLevelString() );
            daxs->type = IData::READ;
            dlist->push_back(daxs);
        }
        daxs = new DataAccess;
        daxs->data = d3;
        daxs->required_version = d3->getReadVersion();
        daxs->required_version.setContext( glbCtx.getLevelString() );
        d3->getWriteVersion().setContext( glbCtx.getLevelString() );
        d3->getReadVersion().setContext( glbCtx.getLevelString() );
        daxs->type = IData::WRITE;
        dlist->push_back(daxs);

        //TaskHandle task_handle = /todo
        dtEngine.addTask(ctx,s,key,d3->getHost(),dlist);

        if (d1) d1->dump(' ');
        if (d2) d2->dump(' ');
        d3->dump(' ');

    }

    if ( d1 != NULL ) d1->incrementVersion(IData::READ);
    if ( d2 != NULL ) d2->incrementVersion(IData::READ);
    d3->incrementVersion(IData::WRITE);

    delete dr;
    c->clear();
    delete c;
}
void AddTask ( IContext *ctx,string s,unsigned long key,IData *d1                    )
{
    AddTask ( ctx,s,key,NULL,NULL , d1);
}
void AddTask ( IContext *ctx,string s,unsigned long key,IData *d1,IData *d2          )
{
    AddTask ( ctx,s,key,d1  ,NULL , d2);
}
/*===============================================================================*/
