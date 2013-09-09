#ifndef __CTX_BNDRY_HPP__
#define __CTX_BNDRY_HPP__

#include <list>
using namespace std;
class IContext;
class ContextBoundry
{
private:
  int s;
  ContextBoundry *parent;
  list<ContextBoundry *> children;
public :
  ContextBoundry(){s=0;}
  void setParent(ContextBoundry *p){
    parent = p;
    p->addChild(this);
  }
  ContextBoundry *getParent(){return parent;}
  void addChild(ContextBoundry *c){
    children.push_back(c);
  }
  int getID() {return s;}
  void setID(int id) { s=id;}
  bool Boundry(IContext* ctx,bool c);
  
};


#endif //__CTX_BNDRY_HPP__
