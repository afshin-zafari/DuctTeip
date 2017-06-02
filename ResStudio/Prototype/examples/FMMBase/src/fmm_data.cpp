#include "fmm_data.hpp"
#include "fmm_context.hpp"
namespace FMM_3D{
  /*------------------------------------------------------------------------*/
  void DTBase::get_dims(int &m , int &n){
      m = M;
      n = N;
    }
  /*------------------------------------------------------------------------*/
    void export_it(fstream &f){}
  /*------------------------------------------------------------------------*/
    void DTBase::allocate(int m , int n){
      long hdr = getHeaderSize();
      M = m;
      N = n;
      lead_dim = M;
      if( getHost() != me )
        return;
      ElementType *p = new ElementType[m*n+hdr/sizeof(ElementType)];
      data =&p[hdr/sizeof(ElementType)];
      std::cout << "(0) Data address: " << data  << std::endl;
      allocateMemory();
    }
  /*------------------------------------------------------------------------*/
    DTBase::DTBase(){
      handle = new DTHandle;
      data_count ++;
      handle->id = last_handle++;
      memory_type = USER_ALLOCATED;
      host_type=ALL_HOST;
      data = nullptr;
      complex_data = nullptr;

      IData::parent_data = NULL;
      setDataHandle( fmm_engine->createDataHandle());
      setDataHostPolicy( glbCtx.getDataHostPolicy() ) ;
      setLocalNumBlocks(1,1);
      IData::Mb = 0;
      IData::Nb = 0;
      setHostType(ALL_HOST);
      setParent(fmm_engine);
      fmm_engine->addInputData(this);      
    }
  /*------------------------------------------------------------------------*/
    DTBase::~DTBase(){
      data_count --;
    }
  /*------------------------------------------------------------------------*/
  void DTBase::setNewMemoryInfo(MemoryItem*mi){
    //lead_dim = mi->getLead();
    M = lead_dim ;
    N = (mi->getSize() -getHeaderSize())/ sizeof(ElementType) / M ;
    data = (ElementType*)(mi->getAddress()+getHeaderSize());
  }
  /*------------------------------------------------------------------------*/
      void DTBase::getExistingMemoryInfo(byte **b, int *s, int *l){
      if ( complex_data !=nullptr){
	*b = ((byte*)complex_data) - getHeaderSize()  ;
	*s = M*N*sizeof(ComplexElementType);
      }
      else{
	std::cout << "(1) Data address: " << data  << std::endl;
	*b = ((byte *)data) - getHeaderSize() ;
	*s = M*N*sizeof(ElementType);
      }
      *l = lead_dim;
    }
  /*----------------------------------------------------------------*/
  void TopLayerData::fill(double v){
  double *d = (double *)((byte *)data+0*getHeaderSize());
    for(int i=0; i<M;i++){
      for(int j=0;j<N;j++){
	      d[j*M+i] = v;
      }
    }
  }
  /*----------------------------------------------------------------*/
  bool TopLayerData::check(double v){
    if ( data == nullptr)
      return false;
    bool check= true;
    double *d = (double *)((byte *)data+0*getHeaderSize());
    for(int i=0; i<M;i++){
      for(int j=0;j<N;j++){
	if ( d[j*M+i] != v){
	  std::cout << "Error in (" << i <<"," << j <<"), expected: " << v <<" actual: "<< data[j*M+i] << std::endl ;
	  check= false;
	}
      }
    }
    if ( check)
      cout << "All elements are correct.\n";
    return check;  
  }
  /*----------------------------------------------------------------*/
  void TopLayerData::dump(){
  double *d = (double *)((byte *)data+0*getHeaderSize());
  cout << "memory: " << d<< endl;
    for(int i=0; i<M;i++){
      for(int j=0;j<N;j++){
	std::cout << d[j*M+i] << "\t\t";
      }
      std::cout << std::endl;
    }
  }  
  /*------------------------------------------------------------------------*/
  void TopLayerData::fill_sequence(double start, double inc){
    for(int i=0; i<M*N;i++){
      data [i] = start + i*inc;
    } 
  }
  /*------------------------------------------------------------------------*/
  bool TopLayerData::check_sequence(double start, double inc){
    bool check = true;
    for(int i=0; i<M*N;i++){
      double v = start + i*inc;
      if ( data [i] != v){
        int j = i/M;
        std::cout << "Error in (" << i <<"," << j <<"), expected: " << v <<" actual: "<< data[i] << std::endl ;
        check = false; 
      }
    }    
    if ( check)
        cout << "All elements are correct.\n";
}
}//namespace FMM_3D
