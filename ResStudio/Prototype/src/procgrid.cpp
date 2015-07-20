
#include "procgrid.hpp"

ProcessGrid::ProcessGrid(int nodes, int _p, int _q,int _g) {
  P = nodes;
  p = _p ;
  q = _q ;
  G = _g;
}
/*-----------------------------------------------*/
int ProcessGrid::getProcessor(int dy,int dx ) {
  int py = dy % p ;
  int px = dx % q ;
  return (py * q + px) ;
}
/*-----------------------------------------------*/
int ProcessGrid::getProcessorCount(){return P;}
/*-----------------------------------------------*/
int ProcessGrid::getGroupCount() {return G;}
/*-----------------------------------------------*/
bool ProcessGrid::isInList(int _p, list<int> *plist){
  for( int i : *plist)
    if ( i == _p )
      return true;
  return false;
}
/*-----------------------------------------------*/
list<int> *ProcessGrid::col(int c ) {
  list<int> *plist= new list<int>;
  for (int i=0;i<p;i++){
    plist->push_back(c%q + i*q);
  }
  return plist;
}
/*-----------------------------------------------*/
list<int> *ProcessGrid::row(int r ) {
  list<int> *plist= new list<int>;
  for (int i=0;i<q;i++){
    plist->push_back((r%p)*q+i);
  }
  return plist;
}
/*-----------------------------------------------*/
bool ProcessGrid::isInRow(int x , int r ) {
  int rp = (r%p);
  return (  (rp*q <= x) && (x < (rp+1)*q) ) ;
}
/*-----------------------------------------------*/
bool ProcessGrid::isInCol(int x , int c ) {
  int cq = c%q;
  return ( cq == (x % q) ) ;
}
