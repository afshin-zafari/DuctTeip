#ifndef __PROCGRID_HPP__
#define __PROCGRID_HPP__

#include <string>
#include <iostream>
#include <vector>
#include <list>
using namespace std;

class ProcessGrid 
{
private:
  int P,p,q;
  int Nb,Mb;
public:
  ProcessGrid(int nodes, int _p, int _q) {
    P = nodes;
    p = _p ;
    q = _q ; 
  }
  int getProcessor(int dy,int dx ) {
    int py = dy % p ; 
    int px = dx % q ;
    return (py * q + px) ;
  }
  bool isInList(int p, list<int> *plist){
    list<int>::iterator it ;
    for ( it = plist->begin(); it != plist->end(); it ++ ) 
      if ( *it == p ) 
	return true;
    return false;
  }
  list<int> *col(int c ) {
    list<int> *plist= new list<int>;
    for (int i=0;i<p;i++){
      plist->push_back(c%q + i*q);
      //cout << c%q+i*q << endl;
    }
    return plist;
  }
  list<int> *row(int r ) {
    list<int> *plist= new list<int>;
    for (int i=0;i<q;i++){
      plist->push_back((r%p)*q+i);
      //cout << r<<"," <<r%p<< "," << ((r%p)*q +i) << endl;
    }
    return plist;
  }
  bool isInRow(int x , int r ) {
    int rp = (r%p);
    return (  (rp*q <= x) && (x < (rp+1)*q) ) ;
  }
  bool isInCol(int x , int c ) {
    int cq = c%q;
    return ( cq == (x % q) ) ;
  }
};

#endif // __PROCGRID_HPP__
