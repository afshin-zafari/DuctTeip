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
  int G,P,p,q;
public:
  ProcessGrid(int nodes, int _p, int _q,int _g =1) ;
  int getProcessor(int dy,int dx ) ;
  int getProcessorCount();
  int getGroupCount();
  bool isInList(int _p, list<int> *plist);
  list<int> *col(int c ) ;
  list<int> *row(int r ) ;
  bool isInRow(int x , int r ) ;
  bool isInCol(int x , int c ) ;
};

#endif // __PROCGRID_HPP__
