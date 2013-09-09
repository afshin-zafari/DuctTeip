#include <string>
#include <iostream>
using namespace std;
class IContext
{
protected: 
  string _name;
public:
  IContext(string name){
    _name=name;
  }
};
class Context: public IContext
{
public :
  Context (string s ) :  IContext(s)
  {
  }
  void print_name(){cout << _name << endl;}
};
int main () 
{
  int i;
  Context MA(static_cast<string>("MatAsm"));
  MA.print_name();

}
