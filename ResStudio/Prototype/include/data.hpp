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
//class IData;
class IHostPolicy ;
class IContext;
//class IListener;
class IDuctteipTask;
class MailBox;
//typedef IListener DataListener;

/*========================== IData Class =====================================*/
class IData
{
protected:
  string                      name;
  int                         N,M,Nb,Mb;
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
public:
  /*--------------------------------------------------------------------------*/
  enum AccessType {    READ  = 1,    WRITE = 2  , SCALAR=64};
  /*--------------------------------------------------------------------------*/
  IData();
   IData(string _name,int m, int n,IContext *ctx);
  ~IData() ;
  /*--------------------------------------------------------------------------*/
  string        getName          ()                { return name;}
  IContext     *getParent        ()                { return parent_context;}
  IData        *getParentData    ()                { return parent_data;}
  void          setParent        (IContext *p)     { parent_context=p;}
  DataVersion  &getWriteVersion  ()                { return gt_write_version;}
  DataVersion  &getReadVersion   ()                { return gt_read_version;}
  IHostPolicy  *getDataHostPolicy()                { return hpData;}
  void          setDataHostPolicy(IHostPolicy *hp) { hpData=hp;}
  void          setDataHandle    ( DataHandle *d)  { my_data_handle = d;}
  DataHandle   *getDataHandle    ()                { return my_data_handle ; }  
  unsigned long getDataHandleID  ()                {return my_data_handle->data_handle;}
  /*--------------------------------------------------------------------------*/
  void allocateMemory();
  void prepareMemory();
  void addTask(IDuctteipTask *);
  void checkAfterUpgrade(list<IDuctteipTask *> &,MailBox *,char debug =' ');
  /*--------------------------------------------------------------------------*/
  byte  *getHeaderAddress();
  double *getContentAddress();
  void setDataMemory(MemoryItem *mi ) ;
  MemoryItem *getDataMemory();
  void setLocalNumBlocks(int mb, int nb ) ;
  /*--------------------------------------------------------------------------*/
  int  getXLocalNumBlocks( ) {    return local_nb;  }
  /*--------------------------------------------------------------------------*/
  int  getXNumBlocks     ( ) {    return Nb;  }
  /*--------------------------------------------------------------------------*/
  int  getYLocalNumBlocks( ) {    return local_mb;  }
  /*--------------------------------------------------------------------------*/
  int  getYNumBlocks     ( ) {    return Mb;  }
  /*--------------------------------------------------------------------------*/
  int  getXLocalDimension( ) {    return local_n ;  }
  /*--------------------------------------------------------------------------*/
  int  getYLocalDimension( ) {    return local_m ;  }
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
  int   getHost();
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
  int getContentSize();
  void setContentSize(long s){content_size =s;}
  int getHeaderSize();
  int getPackSize(); 
  void testHandles();
  bool isOwnedBy(int p ) ;
  void incrementVersion ( AccessType a);
  void dump(char ch=' ');
  string  dumpVersionString();
  void dumpVersion();
  void addToVersion(AccessType axs,int v);
  void setPartition(int _mb, int _nb);
  void resetVersion();
  Coordinate  getBlockIdx(){ return blk;  }
  DataRange  *RowSlice(int r , int i, int j ) ;
  DataRange  *ColSlice(int c , int i, int j ) ;
  DataRange  *Region(int fr, int tr, int fc, int tc ) ;
  DataRange  *Cell(int i, int j ) ;
  DataRange  *All();
  list<IListener *>          &getListeners(){return listeners;}
  list<IDuctteipTask *>      &getTasks(){return tasks_list;}
  void dumpCheckSum(char c='i');
  bool isExportedTo(int p ) ;
  void setExportedTo(int p);
  void setName(string s);
};
/*========================== IData Class =====================================*/

class Data: public IData
{
public:
  Data(string  _name,int n, int m,IContext *ctx):  IData(_name,n,m,ctx){}
};

class DuctTeip_Data : public Data {
public: 
  DuctTeip_Data(int M, int N);  
  void configure();
  DuctTeip_Data(int M, int N,IContext *alg);
  DuctTeip_Data *clone();
};



#endif //__DATA_HPP__
