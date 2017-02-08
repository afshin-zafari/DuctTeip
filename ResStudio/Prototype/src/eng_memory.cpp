#include "engine.hpp"
/*---------------------------------------------------------------*/
MemoryManager *engine::getMemoryManager(){return data_memory;}
/*---------------------------------------------------------------------------------*/
MemoryItem *engine::newDataMemory(){
  assert(data_memory);
  MemoryItem *m = data_memory->getNewMemory();
  return m;
}
/*---------------------------------------------------------------------------------*/
void engine::insertDataMemory(IData *d,byte *mem){
  assert(data_memory);
  MemoryItem *mi = data_memory->insertMemory(mem);
  d->setDataMemory(mi);
  
}
