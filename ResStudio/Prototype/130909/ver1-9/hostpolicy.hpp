#ifndef __HOSTPOLICY_HPP__
#define __HOSTPOLICY_HPP__

#include <string>
#include <iostream>
#include <vector>
#include <list>

typedef struct {
  int bx, by;
} Coordinate;

class ProcessGrid;

class IHostPolicy 
{
protected :
  ProcessGrid *PG;
public:
  IHostPolicy(ProcessGrid *pg):PG(pg){}
  virtual int getHost(Coordinate c) =0;
  virtual bool isInRow(int ,int)=0;
  virtual bool isInCol(int ,int)=0;
  
};
class DataHostPolicy : public IHostPolicy
{
public :
  DataHostPolicy (ProcessGrid *pg) : IHostPolicy(pg){}
  int getHost(Coordinate c ) {
    return PG->getProcessor(c.by,c.bx);
  }
  bool isInRow(int me , int r ) {
    return PG->isInRow(me,r);
  }
  bool isInCol(int me , int c ) {
    return PG->isInCol(me,c);
  }
};

#endif //__HOSTPOLICY_HPP__
