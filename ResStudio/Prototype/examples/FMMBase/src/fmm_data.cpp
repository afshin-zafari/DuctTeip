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
      data = new ElementType[m*n+hdr/sizeof(ElementType)];
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
      fmm_engine->addInputData(this);
    }
  /*------------------------------------------------------------------------*/
    DTBase::~DTBase(){
      data_count --;
    }
  /*------------------------------------------------------------------------*/
    void DTBase::getExistingMemoryInfo(byte **b, int *s, int *l){
      if ( complex_data !=nullptr){
	*b = (byte*)complex_data;
	*s = M*N*sizeof(ComplexElementType);
      }
      else{
	std::cout << "(1) Data address: " << data  << std::endl;
	*b = (byte *)data;
	*s = M*N*sizeof(ElementType);
      }
      *l = lead_dim;
    }
  /*------------------------------------------------------------------------*/
  /*------------------------------------------------------------------------*/
  /*------------------------------------------------------------------------*/
  
}//namespace FMM_3D
