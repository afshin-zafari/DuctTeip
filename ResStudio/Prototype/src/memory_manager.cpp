#ifndef __MEMORY_MANAGER_HPP__ 
#define  __MEMORY_MANAGER_HPP__
#include "basic.hpp"
//#include <list>
#include <vector>

typedef unsigned long MemoryKey;
/*----------------------------------------------------------------------------*/
struct MemoryItem{
private:
  unsigned long  size;
  MemoryKey      key;
  byte          *address;
  int            state;
public:
  enum MemoryState{
    Initialized,
    Allocated,
    InUse,
    Ready,
    Deallocated
  };

/*----------------------------------------------------------------------------*/
  MemoryItem () {
    PRINT_IF(0) ("mitem empty ctor\n");
    address = NULL;
    size    = 0;
    key     = -1;
    state   = Deallocated;
  }
  MemoryItem(int size_,MemoryKey key_){
    address = new byte[size_];
    TRACE_ALLOCATION(size_);
    state = Ready;
    size = size_;
    key = key_;
    PRINT_IF(0)("mitem ctor: ");
    dump();
  }
/*----------------------------------------------------------------------------*/
  ~MemoryItem(){
    PRINT_IF(0)("mitem dtor\n");
    delete[] address;
  }
/*----------------------------------------------------------------------------*/
  void setState(int s ) {state = s;} 
/*----------------------------------------------------------------------------*/
  MemoryKey getKey() { return key;}
/*----------------------------------------------------------------------------*/
  unsigned long getSize() { return size;}
/*----------------------------------------------------------------------------*/
  int getState() { return state;}
/*----------------------------------------------------------------------------*/
  byte *getAddress() { 
    PRINT_IF(0)("mitem getadr:%p\n",address);
    return address;
  }
/*----------------------------------------------------------------------------*/
  void dump(){
    //    if ( DUMP_FLAG)
      PRINT_IF(0)("mitem ,size:%ld,key:%ld,adr:%p,state:%d\n",size,key,address,state);
  }
/*----------------------------------------------------------------------------*/
};
/*============================================================================*/

/*============================================================================*/

class MemoryManager{
private:
  vector <MemoryItem *> memory_list;
  unsigned long       element_size;
  MemoryKey           last_key;
public:
/*----------------------------------------------------------------------------*/
  MemoryManager(int elem_count, int elem_size){
    element_size = elem_size;
    
    last_key = 0 ; 
    PRINT_IF(0)("mmngr ctor, n:%d,size:%d\n",elem_count,elem_size);
    expandMemoryItems(elem_count);
  }
/*----------------------------------------------------------------------------*/
 ~MemoryManager(){
   PRINT_IF(0)("mmngr dtor\n");
   memory_list.clear();
 }

/*----------------------------------------------------------------------------*/
  MemoryItem *getNewMemory(){
    MemoryItem *m = NULL;
    m = findFreeMemory();
    if ( m == NULL ) {
      expandMemoryItems();
      m = findFreeMemory();
    }
    m->setState(MemoryItem::InUse);
    PRINT_IF(0)("mmngr:%p state:%d\n",m->getAddress(),m->getState());
    return m;
  }
/*----------------------------------------------------------------------------*/
  MemoryItem *expandMemoryItems(int n =0){
    MemoryItem * first_mem,*m;
    int elem_count = memory_list.size();
    int expand  = (elem_count ==0)?n:elem_count/2 + 1;
    PRINT_IF(0)("mmngr expand to:%d\n",elem_count+expand);
    for ( int i = 0 ; i < expand; i ++){
      m = new MemoryItem(element_size,last_key ++);
      if ( i == 0 ) 
	first_mem = m ;
      memory_list.push_back(m);
    }    
    return first_mem;
  }
/*----------------------------------------------------------------------------*/
  MemoryItem *findFreeMemory(){
    MemoryItem *m = NULL;
    vector<MemoryItem *>::iterator it;
    for(it = memory_list.begin() ; it != memory_list.end() ; it ++){
      m = (*it);
      if ( m->getState() != MemoryItem::InUse ) {
	PRINT_IF(0)("find free mem :%p state:%d\n",m->getAddress(),m->getState());
	return m;
      }
    }
    return NULL;
  }
/*----------------------------------------------------------------------------*/
  void freeMemoryItem(MemoryItem *m ) {m->setState(MemoryItem::Ready);}
  
};
/*============================================================================*/
#endif // __MEMORY_MANAGER_HPP__
