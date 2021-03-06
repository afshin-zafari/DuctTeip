#include "memory_manager.hpp"
/*----------------------------------------------------------------------------*/
MemoryItem::MemoryItem ()
{
    address = NULL;
    size    = 0;
    key     = -1;
    state   = Deallocated;
}
MemoryItem::MemoryItem(int size_,MemoryKey key_)
{
    address = new byte[size_];
    state = Ready;
    size = size_;
    key = key_;
    dump();
}
MemoryItem::MemoryItem(byte *adr,int size_,int lead_)
{
  address = adr;
    state = Ready;
    size = size_;
    key = -1;
    leading_dim = lead_;
    dump();
}
/*----------------------------------------------------------------------------*/
MemoryItem::~MemoryItem()
{
  if(key !=-1)
    delete[] address;
}
/*----------------------------------------------------------------------------*/
void MemoryItem::setState(int s )
{
    state = s;
}
/*----------------------------------------------------------------------------*/
MemoryKey MemoryItem::getKey()
{
    return key;
}
/*----------------------------------------------------------------------------*/
unsigned long MemoryItem::getSize()
{
    return size;
}
/*----------------------------------------------------------------------------*/
int MemoryItem::getState()
{
    return state;
}
/*----------------------------------------------------------------------------*/
byte *MemoryItem::getAddress()
{
    return address;
}
/*----------------------------------------------------------------------------*/
void MemoryItem::setAddress(byte  *mem)
{
    address = mem;
}
/*----------------------------------------------------------------------------*/
void MemoryItem::dump()
{
}
/*----------------------------------------------------------------------------*/

/*============================================================================*/

/*============================================================================*/

/*----------------------------------------------------------------------------*/
MemoryManager::MemoryManager(int elem_count, int elem_size)
{
    element_size = elem_size;

    last_key = 0 ;
    expandMemoryItems(elem_count);
}
/*----------------------------------------------------------------------------*/
MemoryManager::~MemoryManager()
{
    memory_list.clear();
}

/*----------------------------------------------------------------------------*/
MemoryItem * MemoryManager::insertMemory(byte  *mem){
  MemoryItem *mi = getNewMemory();
  byte *old = mi->getAddress();
  delete old;
  mi->setAddress(mem);
  return mi;
}
/*----------------------------------------------------------------------------*/
MemoryItem *MemoryManager::getNewMemory()
{
    MemoryItem *m = NULL;
    m = findFreeMemory();
    if ( m == NULL )
    {
        expandMemoryItems();
        m = findFreeMemory();
    }
    assert(m);
    m->setState(MemoryItem::InUse);
    return m;
}
/*----------------------------------------------------------------------------*/
MemoryItem *MemoryManager::expandMemoryItems(int n )
{
    MemoryItem * first_mem,*m;
    first_mem=NULL;
    int elem_count = memory_list.size();
    int expand  = (elem_count ==0)?n:10;//elem_count/2 + 1;
    fprintf(stderr,"@memory pool is going to be expanded by %d items, each %ld bytes.\n",expand,element_size);
    for ( int i = 0 ; i < expand; i ++)
    {
        m = new MemoryItem(element_size,last_key ++);
	assert(m);
        if ( i == 0 )
            first_mem = m ;
        memory_list.push_back(m);
    }
    assert(first_mem);
    return first_mem;
}
/*----------------------------------------------------------------------------*/
MemoryItem *MemoryManager::findFreeMemory()
{
    MemoryItem *m = NULL;
    for(uint i=0; i<memory_list.size(); i++)
    {
        m=memory_list[i];
	assert(m);
        if ( m->getState() != MemoryItem::InUse )
        {
            return m;
        }
    }
    return NULL;
}
/*----------------------------------------------------------------------------*/
void MemoryManager::freeMemoryItem(MemoryItem *m )
{
  assert(m);
  m->setState(MemoryItem::Ready);
}
