#ifndef __PARTITION_HPP__
#define __PARTITION_HPP__
#include "superglue.hpp"
#include "option/instr_tasktiming.hpp"

class IData;

/*===================== DataBlock =======================================================*/

template <class ElementType>
class Partition  { 
private:
  struct MemoryUnit { int count, stride;};
  struct Dimension  { 
    MemoryUnit elements,blocks;
    int elements_per_block;
  };
  enum MemoryAlignment {ROW_MAJOR,COL_MAJOR};
  enum DimensionIndex  {X_AXIS,Y_AXIS};
  Dimension        *dimensions;
  ElementType      *memory;
  int               dim_count,mem_size;
  MemoryAlignment   block_alignment, element_alignment;
public:
  Partition ( int dim_cnt=2 ,MemoryAlignment algn=ROW_MAJOR){
    dimensions = new Dimension  [dim_cnt];
    dim_count = dim_cnt;
    element_alignment = algn;
    block_alignment=ROW_MAJOR;
  }
  /*--------------------------------------------------------------------------*/
  ~Partition () {
    //printf ("partition dtor\n");
    delete[] dimensions;
  }
  /*--------------------------------------------------------------------------*/
  inline int &X_E () {return  dimensions[X_AXIS].elements.count ;}    // Number of elements in X dimension
  inline int &X_B () {return  dimensions[X_AXIS].blocks  .count ;}    // Number of blocks in X dimension
  inline int &X_ES() {return  dimensions[X_AXIS].elements.stride;}    // Elements stride in X dimension
  inline int &X_EB() {return  dimensions[X_AXIS].elements_per_block;}
  inline int &X_BS() {return  dimensions[X_AXIS].blocks  .stride;}    // Blocks stride in X dimension
  inline int &Y_EB() {return  dimensions[Y_AXIS].elements_per_block;}
  inline int &Y_ES() {return  dimensions[Y_AXIS].elements.stride;}    // Elements stride in Y  dimension
  inline int &Y_BS() {return  dimensions[Y_AXIS].blocks  .stride;}    // Blocks stride in Y dimension
  inline int &Y_E () {return  dimensions[Y_AXIS].elements.count ;}    // Number of elements in Y dimension
  inline int &Y_B () {return  dimensions[Y_AXIS].blocks  .count ;}    // Number of blocks in Y dimension
  /*--------------------------------------------------------------------------*/
  ElementType * getAddressOfBlock(int y , int x ) {
    if ( dim_count > 2 ) {
      printf ("error: No. of dimensions are more than two.\n");
      return NULL;
    }
    if ( !memory ) 
      return NULL;
    Partition<ElementType> *P = getBlock( x,y);
    ElementType *m = P->memory;
    delete P;
    return m;
  } 
  /*--------------------------------------------------------------------------*/
  void dump(){
    if ( !DUMP_FLAG)
      return;
    printf("xe:%d,ye:%d, xeb:%d, yeb:%d mem:%p\n",X_E(),Y_E(),X_EB(),Y_EB(),memory);
    for ( int i = 0 ; i < X_E() ; i ++ ) {
      if ( i % Y_EB()  == 0 && i ) 
	printf ("\n");
      for ( int j = 0 ; j < Y_E() ; j ++) {
	if ( j % X_EB() == 0  && j )
	  printf ( ", " ) ;
	printf ("%5.1lf ", memory[i * X_E() + j ]);
      }
      printf ("\n");
    }
  }
  /*--------------------------------------------------------------------------*/
  void dumpBlocks(){
    for(int i=0;i < Y_B(); i++){
      for ( int j=0;j<X_B();j++){
	Partition *p=getBlock(i,j);
	printf("Block(%d,%d):\n",i,j);
	p->dump();
	delete p;
      }
    }
  }
  /*--------------------------------------------------------------------------*/
  Partition *getBlock ( int y, int x ) ;
  /*--------------------------------------------------------------------------*/
  ElementType  *getElementAt(int y , int x ) {
    ElementType *elem = getAddressOfElement(y,x);
    return elem;
  }
  /*--------------------------------------------------------------------------*/
  ElementType * getElement(int y , int x ) {
    
    if ( element_alignment == ROW_MAJOR ) {
      return memory + y * X_E() + x ; 
    }
    else{
      return memory + x * Y_E() + y ; 
    }
    printf ("error: Partition memory is NULL and getElement is called.\n"); 
    assert ( !memory ) ; 
    return NULL;
  } 
  /*--------------------------------------------------------------------------*/
  ElementType * getAddressOfElement(int y , int x ) {
    if ( x > X_E() ) 
      return NULL;
    if ( y > Y_E() ) 
      return NULL;
    Partition *P = getBlock(y/Y_EB() , x/X_EB() ) ;
    ElementType *elem = P->getElement( y % Y_EB() , x % X_EB()  ) ;
    delete P;
    return elem;
  }
  /*--------------------------------------------------------------------------*/
  void partitionSquare( int n_elem, int n_blocks) {
    setElementsInfoX(n_elem);
    setElementsInfoY(n_elem,n_elem);
    if ( element_alignment == ROW_MAJOR ) {
      setBlocksInfoX(n_blocks,n_elem/n_blocks);
      setBlocksInfoY(n_blocks,n_elem * n_elem/n_blocks);
    }
    else {//todo 
    }
    X_EB() = X_E() / X_B();
    Y_EB() = Y_E() / Y_B();
  }
  /*--------------------------------------------------------------------------*/
  void partitionRectangle( int yn_elem, int xn_elem,int yn_blocks,int xn_blocks) {
    setElementsInfoX(xn_elem);
    setElementsInfoY(yn_elem,-1);
    if ( element_alignment == ROW_MAJOR ) {
      setBlocksInfoX(xn_blocks,-1);
      setBlocksInfoY(yn_blocks,-1);
    }
    else {//todo 
    }
    X_EB() = X_E() / X_B();
    Y_EB() = Y_E() / Y_B();
    PRINT_IF(0)("yeb:%d,ye/yb:%d,xeb:%d,xe/xb:%d\n",Y_EB(),Y_E()/Y_B(),X_EB(),X_E()/X_B());
  }
  /*--------------------------------------------------------------------------*/
  void setElementsInfoX (int count, int stride=0 ) {
    if ( stride == 0 ) 
      if ( element_alignment == ROW_MAJOR ) 
	stride = 1;
      else 
	printf ("error: X_stride is 0 for Column  Major alignment.\n");
    setElementsInfo ( X_AXIS,count, stride ) ;
  }
  /*--------------------------------------------------------------------------*/
  void setElementsInfoY (int count, int stride=0 ) {
    if ( stride == 0 ) 
      if ( element_alignment == COL_MAJOR ) 
	stride = 1;
      else 
	printf ("error: Y_stride is 0 for Row Major alignment.\n");
    setElementsInfo ( Y_AXIS,count, stride ) ;
  }
  /*--------------------------------------------------------------------------*/
  void setElementsInfo ( int dim,int count, int stride ) {
    dimensions[dim].elements.count  = count ; 
    dimensions[dim].elements.stride = stride ; 
  }
  /*--------------------------------------------------------------------------*/
  void setBlocksInfoX (  int count, int stride ) {
     setBlocksInfo ( X_AXIS,  count,  stride ) ;
  }
  /*--------------------------------------------------------------------------*/
  void setBlocksInfoY (  int count, int stride ) {
     setBlocksInfo ( Y_AXIS,  count,  stride ) ;
  }
  /*--------------------------------------------------------------------------*/
  void setBlocksInfo ( int dim, int count, int stride ) {
    dimensions[dim].blocks.count  = count ; 
    dimensions[dim].blocks.stride = stride ; 
  }
  /*--------------------------------------------------------------------------*/
  void setBaseMemory ( ElementType *mem, int size ) {
    memory = mem;mem_size = size;  
  }
  /*--------------------------------------------------------------------------*/
  ElementType *getBaseMemory () {    return memory;}
  int          getMemorySize () {    return mem_size;}
  /*--------------------------------------------------------------------------*/

};
/*===================== DataBlock =======================================================*/
/*===================== SuperGlue Handle ================================================*/
template<typename Options>
struct MyHandle : public HandleDefault<Options> {
  Partition<double> *block;
  char name[20];
  ~MyHandle(){
    if(block) 
      delete block;
    block = NULL;
  }
  MyHandle() {
  }
};

struct Options : public DefaultOptions<Options> {
    typedef MyHandle<Options> HandleType;
    typedef Enable Logging;
    typedef TaskExecutorTiming<Options> TaskExecutorInstrumentation;
};
/*===================== SuperGlue Handle ================================================*/

#endif // __PARTITION_HPP__