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
/*----------------------------------------------------------------------------*/
MemoryItem::~MemoryItem()
{
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
    int expand  = (elem_count ==0)?n:elem_count/2 + 1;
    for ( int i = 0 ; i < expand; i ++)
    {
        m = new MemoryItem(element_size,last_key ++);
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
    m->setState(MemoryItem::Ready);
}
