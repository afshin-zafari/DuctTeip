#ifndef __PARTITION_HPP__
#define __PARTITION_HPP__
#include "sg/superglue.hpp"
#include "sg/core/contrib.hpp"
#include "sg/option/instr_trace.hpp"
#include "basic.hpp"
//class IData;

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
  Partition ( int dim_cnt=2 ,MemoryAlignment algn=COL_MAJOR){
    dimensions = new Dimension  [dim_cnt];
    dim_count = dim_cnt;
    element_alignment = algn;
    block_alignment=algn;
  }
  /*--------------------------------------------------------------------------*/
  ~Partition () {
    if (dimensions != NULL) {
      printf ("partition dtor,dim:%p\n",dimensions);
      delete[] dimensions;
      printf ("partition dtor\n");
      dimensions = NULL;
    }
    printf ("end partition dtor\n");
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
#if BUILD ==RELEASE
    return;
#else
    return;
    fprintf(stderr,"xe:%d,ye:%d, xeb:%d, yeb:%d mem:%p\n",X_E(),Y_E(),X_EB(),Y_EB(),memory);
    for ( int i = 0 ; i < X_E() ; i ++ ) {
      if ( i % Y_EB()  == 0 && i )
	fprintf (stderr,"\n");
      for ( int j = 0 ; j < Y_E() ; j ++) {
	if ( j % X_EB() == 0  && j )
	  fprintf (stderr, ", " ) ;
	fprintf (stderr,"%5.1lf ", memory[i * X_E() + j ]);
      }
      fprintf (stderr,"\n");
    }
#endif
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
  Partition *getBlock ( int y, int x ) {

    Partition<ElementType> *p = new Partition<ElementType> (dim_count, element_alignment) ;
    p->block_alignment = block_alignment ;
    p->mem_size = mem_size;
    p->X_E()  = X_EB();
    p->Y_E()  = Y_EB();

    p->X_B()  = 1;
    p->Y_B()  = 1;

    p->Y_BS() = 0;
    p->X_BS() = 0;

    p->X_EB() = p->X_E() / p->X_B();
    p->Y_EB() = p->Y_E() / p->Y_B();

    if ( element_alignment == ROW_MAJOR ) {
      p->X_ES() = 1;
      p->Y_ES() = X_EB();
    }
    else{
      p->X_ES() = Y_EB();
      p->Y_ES() = 1     ;
    }
    if ( block_alignment  == ROW_MAJOR ) {
      p->memory = memory + (y * X_B() + x )* Y_EB() * X_EB() ;
    }
    else {
      p->memory = memory + (x * Y_B() + y )* Y_EB() * X_EB() ;
    }
    if(0)printf("yeb %d e %d b %d\n",p->Y_EB(),p->Y_E(),p->Y_B());
    return p;
  }
  /*--------------------------------------------------------------------------*/
  ElementType  *getElementAt(int y , int x ) {
    if(0)printf("Element At %d,%d\n",x,y);
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
    if(0)printf("x,y :%d,%d _E %d,%d _EB %d,%d\n",x,y,X_E(),Y_E(), X_EB(),Y_EB());
    if ( x > X_E() )
      return NULL;
    if ( y > Y_E() )
      return NULL;
//    Partition *P = getBlock(y/Y_EB() , x/X_EB() ) ;
    int by = y / Y_EB();
    int bx = x / X_EB();
    int offset ;
    ElementType *elemp;
    if ( element_alignment == ROW_MAJOR ) {
      offset= (by * X_B() + bx )  * X_EB() * Y_EB();
      elemp = memory + offset + (y%Y_EB() ) * X_EB() + (x % X_EB()) ;
    }
    else{
      offset= (bx * Y_B() + by )  * X_EB() * Y_EB();
      elemp = memory + offset + (x%X_EB() ) * Y_EB() + (y % Y_EB()) ;
    }
    return elemp;
  }
  /*--------------------------------------------------------------------------*/
  void partitionSquare( int n_elem, int n_blocks) {
    if ( element_alignment == ROW_MAJOR ) {
      setElementsInfoX(n_elem);
      setElementsInfoY(n_elem,n_elem);
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
    if ( element_alignment == ROW_MAJOR ) {
      setElementsInfoX(xn_elem);
      setElementsInfoY(yn_elem,-1);
      setBlocksInfoX(xn_blocks,-1);
      setBlocksInfoY(yn_blocks,-1);
    }
    else {
      setElementsInfoX(xn_elem,yn_elem);
      setElementsInfoY(yn_elem,1);
      setBlocksInfoX(xn_blocks,1);
      setBlocksInfoY(yn_blocks,1);
    }
    X_EB() = X_E() / X_B();
    Y_EB() = Y_E() / Y_B();
    LOG_INFO(0&LOG_DATA,"yeb:%d,ye/yb:%d,xeb:%d,xe/xb:%d\n",Y_EB(),Y_E()/Y_B(),X_EB(),X_E()/X_B());
  }
  /*--------------------------------------------------------------------------*/
  void setElementsInfoX (int count, int stride=0 ) {
    static bool once=false;
    if ( stride == 0 ) {
      if ( element_alignment == ROW_MAJOR )
        stride = 1;
      else{
	if(!once){
	  printf ("error: X_stride is 0 for Column  Major alignment.\n");
	  once=true;
	}
      }
    }
    setElementsInfo ( X_AXIS,count, stride ) ;
  }
  /*--------------------------------------------------------------------------*/
  void setElementsInfoY (int count, int stride=0 ) {
    if ( stride == 0 ){
      if ( element_alignment == COL_MAJOR )
        stride = 1;
      else
        printf ("error: Y_stride is 0 for Row Major alignment.\n");
	}
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
/*
template <class ElementType>
Partition<ElementType> *
Partition<ElementType>::getBlock ( int y, int x ) {

  Partition<ElementType> *p = new Partition<ElementType> (dim_count, element_alignment) ;
  p->block_alignment = block_alignment ;
  p->mem_size = mem_size;
  p->X_E()  = X_EB();
  p->Y_E()  = Y_EB();

  p->X_B()  = 1;
  p->Y_B()  = 1;

  p->Y_BS() = 0;
  p->X_BS() = 0;

  p->X_EB() = p->X_E() / p->X_B();
  p->Y_EB() = p->Y_E() / p->Y_B();

  if ( element_alignment == ROW_MAJOR ) {
    p->X_ES() = 1;
    p->Y_ES() = X_EB();
  }
  else{
    p->X_ES() = Y_EB();
    p->Y_ES() = 1     ;
  }
  if ( block_alignment  == ROW_MAJOR ) {
    p->memory = memory + (y * X_B() + x )* Y_EB() * X_EB() ;
  }
  else {
    p->memory = memory + (x * Y_B() + y )* Y_EB() * X_EB() ;
  }
  if(0)printf("yeb %d e %d b %d\n",p->Y_EB(),p->Y_E(),p->Y_B());
  return p;
}
*/
/*===================== SuperGlue Handle ================================================*/
template<typename Options>
struct MyHandle : public HandleBase<Options> {
  Partition<double> *block;
  char name[20];
  ~MyHandle(){
    if(block)
      delete block;
    block = NULL;
  }
  MyHandle() {
    block = NULL;
  }
};

struct Options : public DefaultOptions<Options> {
  typedef MyHandle<Options> HandleType;
  typedef Enable Logging;
  typedef Trace<Options> Instrumentation;
  typedef Enable TaskName;
  typedef Enable PassTaskExecutor;
  typedef Enable Subtasks;
};
/*===================== SuperGlue Handle ================================================*/

#endif // __PARTITION_HPP__
