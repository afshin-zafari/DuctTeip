#include "hostpolicy.hpp"  
#include "procgrid.hpp"
int IHostPolicy::getHost(Coordinate c ,int ndim) {
  if ( ndim == 1 )
    return (c.bx % PG->getProcessorCount() ) ;
  return PG->getProcessor(c.by,c.bx);
}
bool IHostPolicy::isInRow(int me , int r ) {
  return PG->isInRow(me,r);
}
bool IHostPolicy::isInCol(int me , int c ) {
  return PG->isInCol(me,c);
}
