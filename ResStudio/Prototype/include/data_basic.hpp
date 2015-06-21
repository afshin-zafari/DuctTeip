#ifndef __DATA_BASIC_HPP__
#define __DATA_BASIC_HPP__
#include "basic.hpp"
#include <list>
using namespace std;
class IData;
/*===================ContextPrefix============================================*/
class  ContextPrefix{
public:
  list<int> context_id_list;
  ContextPrefix(){  }
  ContextPrefix(string s){    fromString(s);  }
  ~ContextPrefix(){    reset();  }
  /*--------------------------------------------------------------------------*/
  int getPackSize(){    return 10* sizeof(int);  }
  /*--------------------------------------------------------------------------*/
  int serialize(byte *buffer,int &offset,int max_length);
  int deserialize(byte *buffer,int &offset,int max_length);
  bool operator !=(ContextPrefix rhs);
  bool operator ==(ContextPrefix rhs);
  void fromString(string ctx);
  ContextPrefix operator=(ContextPrefix rhs);
  string toString();
  void dump();
  void reset(){    context_id_list.clear();  }
};
/*==========================ContextPrefix=====================================*/

/*==========================DataVersion ======================================*/
struct DataVersion{
public:  
  ContextPrefix prefix;
  int version;
  /*--------------------------------------------------------------------------*/
  int getVersion(){    return version;  }
  /*--------------------------------------------------------------------------*/
  DataVersion(){    version =0 ;  }
  ~ DataVersion(){  }
  int getPackSize(){    return prefix.getPackSize() + sizeof(version);  }
  DataVersion operator +=(int a);
  DataVersion operator =(DataVersion rhs);
  DataVersion operator =(int a);
  bool operator ==(DataVersion rhs);
  bool operator <(DataVersion rhs);
  bool operator !=(DataVersion &rhs);
  DataVersion operator ++(int a) ;
  DataVersion operator ++() ;
  string dumpString();
  void dump(char c=' ');
  int   serialize(byte *buffer,int &offset,int max_length);
  int deserialize(byte *buffer,int &offset,int max_length);
  void reset();
  void setContext(string s ){    prefix.fromString(s);  }
};
/*==========================DataVersion ======================================*/

/*============================================================================*/
class Coordinate
{
public :
  int by,bx;
  Coordinate(int i , int j ) : by(i),bx(j){}
  Coordinate( ) : by(0),bx(0){}
};//Coordinate

/*============================================================================*/

typedef struct {
  IData *d;
  int row_from,row_to,col_from,col_to;
}DataRange;


/*==========================DataHandle =======================================*/
struct DataHandle{
public:
  unsigned long int context_handle,data_handle;
  DataHandle () {context_handle=data_handle=0;}
  DataHandle (unsigned long int c,unsigned long int d );
  int getPackSize(){    return sizeof ( context_handle ) + sizeof(data_handle);  }
  
  DataHandle &operator = ( DataHandle rhs ) ;
  bool operator == ( DataHandle rhs ) ;
  int    serialize(byte *buffer, int &offset,int max_length);
  void deserialize(byte *buffer, int &offset,int max_length);
};
/*==========================DataHandle =======================================*/
/*======================= Dataaccess =========================================*/
struct DataAccess{
  IData *data;
  DataVersion required_version;
  byte type;
  int getPackSize(){
    DataHandle dh;
    return dh.getPackSize() + required_version.getPackSize() ;
  }   
} ;

#endif //__DATA_BASIC_HPP__
