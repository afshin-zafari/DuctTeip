#include <iostream>
#include <vector>
#include <list>

using namespace std;
class IDataBlock
{
  public:
};
class IDataHandle
{
  public:
};

class IContext
{
  public:
};
class IData
{
private :
  string name;
public :
  IContext parentContext;
  IDataBlock   getBlock(int i,int j);
  IDataHandle  getHandle();
  void 	      setHandle();
  int getHost();
  void setHost();
  int getVersion();
  void setVersion();
  void incrementVersion();

};
class IAlgorithm
{
  public:
  virtual void generateTasks()=0;

};
class CholeskyAlg: public IAlgorithm {};
class MatrixAssembly: public IAlgorithm {};

class IDuctTeip
{
  public:
  enum DTvent {E1,E2};
  void addTask ();
  void mainCylce();
  void notifyEvent();
  void getPerfMeasures();

};
class IConfiguration
{
  public:

};
class IDataAccess
{
  public:
  
};
class ITask
{
  list<IDataAccess> listAccess;
  public:
  void kernel();
  

};
class ICommunication
{
  public:
  void asyncSend();
  void asyncRecv();
  void probeRecv();
  void testRequest();
};
class IMailBox
{
  public:
  void checkInbox();
  void checkOutbox();

};
class IListener
{
  public:
 

};
class IPerfMeasures
{
  public:
};
int main()
{
}

