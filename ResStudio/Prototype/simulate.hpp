#ifndef __SIMULATE_HPP__
#define __SIMULATE_HPP__
#include <sstream>
/*=========================================*/
class Data {
private:
  int v;
  string name;
public :
  Data(char *s){
    v=0;
    name=string(s);
  }  
  /*-----------------------*/
  void inc(){
    ctx_mngr.dataTouched(this);
    v++;
  }
  /*-----------------------*/
  string ver(){
    stringstream ss;
    ss << ctx_mngr.getCurrentCtx() << '.' << v;
    return ss.str();
    
  }
  /*-----------------------*/
  const char *Name(){
    return name.c_str();
  }
  /*-----------------------*/
  void reset(){v=0;}
  /*-----------------------*/
};
/*=========================================*/
void task(Data &d,Data &e){
  d.inc();
  e.inc();
  printf("                                    %s %s, %s  %s\n",
	 d.Name(),d.ver().c_str(),
	 e.Name(),e.ver().c_str());
}
#endif// __SIMULATE_HPP__
