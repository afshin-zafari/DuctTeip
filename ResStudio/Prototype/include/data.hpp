#ifndef __DATA_HPP__
#define __DATA_HPP__

#include <string>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <new>
#include "basic.hpp"
#include "partition.hpp"
#include "memory_manager.hpp"
#include "dt_log.hpp"
#include "data_basic.hpp"
#include "hostpolicy.hpp"
#include "listener.hpp"
using namespace std;

extern int me;
class IHostPolicy ;
class IContext;
class IDuctteipTask;
class MailBox;

/*========================== IData Class =====================================*/
class IData
{
protected:
    string                      name;
    int                         N,M,Nb,Mb,host;
  int                         memory_type,host_type,leading_dim;
    Coordinate                  blk;
    vector< vector<IData*> >   *dataView;
    DataVersion                 gt_read_version,gt_write_version;
    DataVersion                 rt_read_version,rt_write_version;
    IContext                   *parent_context;
    IData                      *parent_data;
    IHostPolicy                *hpData;
    DataHandle                 *my_data_handle;
    MemoryItem                 *data_memory;
    list<IListener *>          listeners;
    list<IDuctteipTask *>      tasks_list;
    int                        local_n, local_m,local_nb,local_mb,content_size;
    Partition<double>          *dtPartition;
    Handle<Options>            **hM;
    bool                       partial;
    list <int>                exported_nodes;
    void                      *guest;
    vector<int>               host_list;
private:
public:
    /*--------------------------------------------------------------------------*/
    enum AccessType {    READ  = 1,    WRITE = 2  , SCALAR=64};
    enum MemoryType { SYSTEM_ALLOCATED, USER_ALLOCATED};
    enum HostType   { SINGLE_HOST, MULTI_HOST, ALL_HOST};
    /*--------------------------------------------------------------------------*/
    IData();
    IData(string _name,int m, int n,IContext *ctx);
    ~IData() ;
    /*--------------------------------------------------------------------------*/
    string get_name(){return getName();}
    int get_rows(){return M;}
    string        getName          ()
    {
        return name;
    }
    IContext     *getParent        ()
    {
        return parent_context;
    }
    IData        *getParentData    ()
    {
        return parent_data;
    }
    void          setParent        (IContext *p)
    {
        parent_context=p;
    }
  void setParentData(IData *);
    DataVersion  &getWriteVersion  ()
    {
        return gt_write_version;
    }
    DataVersion  &getReadVersion   ()
    {
        return gt_read_version;
    }
    IHostPolicy  *getDataHostPolicy()
    {
        return hpData;
    }
    void          setDataHostPolicy(IHostPolicy *hp)
    {
        hpData=hp;
    }
    void          setDataHandle    ( DataHandle *d)
    {
        my_data_handle = d;
    }
    DataHandle   *getDataHandle    ()
    {
        return my_data_handle ;
    }
    unsigned long getDataHandleID  ()
    {
        return my_data_handle->data_handle;
    }
    void setBlockIdx(int y,int x);
    void getBlockIdx(int &row, int & col)
    {
        row = blk.by;
        col = blk.bx;
    }
    /*--------------------------------------------------------------------------*/
    void allocateMemory();
    void prepareMemory();
  virtual void getExistingMemoryInfo(byte **, int *, int *){}
    void addTask(IDuctteipTask *);
    void checkAfterUpgrade(list<IDuctteipTask *> &,MailBox *,char debug =' ');
    /*--------------------------------------------------------------------------*/
    byte  *getHeaderAddress();
    double *getContentAddress();
    void setDataMemory(MemoryItem *mi ) ;
    MemoryItem *getDataMemory();
    void setLocalNumBlocks(int mb, int nb ) ;
    /*--------------------------------------------------------------------------*/
    int  getXLocalNumBlocks( )
    {
        return local_nb;
    }
    /*--------------------------------------------------------------------------*/
    int  getXNumBlocks     ( )
    {
        return Nb;
    }
    /*--------------------------------------------------------------------------*/
    int  getYLocalNumBlocks( )
    {
        return local_mb;
    }
    /*--------------------------------------------------------------------------*/
    int  getYNumBlocks     ( )
    {
        return Mb;
    }
    /*--------------------------------------------------------------------------*/
    int  getXLocalDimension( )
    {
        return local_n ;
    }
    /*--------------------------------------------------------------------------*/
    int  getYLocalDimension( )
    {
        return local_m ;
    }
    /*--------------------------------------------------------------------------*/
    Handle<Options> **createSuperGlueHandles();
    bool isDataSent(int _host , DataVersion version);
    /*--------------------------------------------------------------------------*/
    void dataIsSent(int _host) ;
    /*--------------------------------------------------------------------------*/
    void listenerAdded(IListener *,int host , DataVersion version ) ;
    /*--------------------------------------------------------------------------*/
    DataVersion getRunTimeVersion(byte type);
    void setRunTimeVersion(string to_ctx, int to_version);
    void incrementRunTimeVersion(byte type,int v = 1 );
    void setHost(int h){host = h;}
    IData *operator () (const int i,const int j=0) ;
    IData *operator [] (const int i) ;
    IData *operator & (void) ;
    double getElement(int row , int col =0) ;
    void setElement(int row, int col ,double v) ;
    void dumpElements();
    IData *getDataByHandle(DataHandle *in_dh ) ;
    void serialize(byte *buffer , int &offset, int max_length);
    byte *serialize();
    void deserialize(byte *buffer, int &offset,int max_length,MemoryItem *mi,bool header_only = true);
    int  getContentSize();
    void setContentSize(long s)
    {
        content_size =s;
    }
    int  getHeaderSize();
    int  getPackSize();
    void testHandles();
    bool isOwnedBy(int p ) ;
    void incrementVersion ( AccessType a);
    void dump(char ch=' ');
    string  dumpVersionString();
    void dumpVersion();
    void addToVersion(AccessType axs,int v);
    void setPartition(int _mb, int _nb);
    void resetVersion();
    Coordinate  getBlockIdx()
    {
        return blk;
    }
    list<IListener *>          &getListeners()
    {
        return listeners;
    }
    list<IDuctteipTask *>      &getTasks()
    {
        return tasks_list;
    }
    void dumpCheckSum(char c='i');
    bool isExportedTo(int p ) ;
    void setExportedTo(int p);
    void setName(string s);
    void *get_guest();
    void set_guest(void *);
    int  getHost();
    void addToHosts(int p);
  void removeFromHosts(int );
  void clearHosts();
    void allocate_memory_for_utp(void);
    void setPartition_for_utp(IData *,int ,int);
};
/*========================== IData Class =====================================*/

class Data: public IData
{
public:
  Data(string  _name,int n, int m,IContext *ctx):  IData(_name,n,m,ctx) {}
  Data():IData(){}
};
/*========================= =====================================*/
class LastLevel_Data {
private :
  int N,M;
  double *memory;
public:

  LastLevel_Data(Task<Options,-1> *t,int arg){
    arg++;
    N = t->getAccess(arg).getHandle()->block->X_E();
    M = t->getAccess(arg).getHandle()->block->Y_E();
    memory = t->getAccess(arg).getHandle()->block->getBaseMemory();
  }
  int get_rows_count(){return M;}
  int get_cols_count(){return N;}
  double *get_memory(){return memory;}
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

/*========================= =====================================*/
  class SuperGlue_Data {
  private:
    Handle<Options> **hM;
    int rows,cols;
  public:
    SuperGlue_Data(IData *d, int &m,int &n){
      hM=d->createSuperGlueHandles();
      rows = m = d->getYLocalNumBlocks();
      cols = n = d->getXLocalNumBlocks();
    }
    int get_rows_count(){return rows;}
    int get_cols_count(){return cols;}
    Handle<Options> &operator()(int i,int j ){
      return hM[i][j];
    }
    ~SuperGlue_Data(){
      cout << "xxxxxx" << endl;
    }
  };

class DuctTeip_Data : public Data
{
public:
  DuctTeip_Data(){}
    DuctTeip_Data(int M, int N);
    DuctTeip_Data(int M, int N,IContext *alg);
    void configure();
    DuctTeip_Data *clone();
    SuperGlue_Data *getSuperGlueData(){
      int m,n;
      SuperGlue_Data *sgd= new SuperGlue_Data(this,m,n);
      return sgd;
  }
};

/*========================= =====================================*/


#endif //__DATA_HPP__
