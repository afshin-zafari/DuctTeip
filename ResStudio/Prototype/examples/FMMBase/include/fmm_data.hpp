#ifndef FMM_DATA_HPP 
#define FMM_DATA_HPP 
#include <complex>
#include <string>
#include <list>
#include "ductteip.hpp"
#include "fmm_types.hpp"

using namespace std;
namespace FMM_3D {
  /*--------------------------------------------------------------------*/
  class DTBase: public IData {
  public:
    static long last_handle,data_count;
    int lead_dim,type,g_index;
    union{
      ElementType *mem,*data;
    };
    ComplexElementType *complex_data;
    DTHandle *handle;
    string name;
    DTBase();
    ~DTBase();
    void get_dims(int &m , int &n);
    void export_it(fstream &f){}
    void allocate(int m , int n);
    void getExistingMemoryInfo(byte **b, int *s, int *l);
    void setNewMemoryInfo(MemoryItem*);
  };
  /*======================================================================*/
  struct Box{int index,level;};
  typedef  vector<Box*> BoxList;
  class TopLayerData: public DTBase{
  private:
    int y,x;
    uint32_t part_no,ng,np;
    vector<TopLayerData*> groups,parts;
    typedef TopLayerData GData;
    /*---------------------------------------------------*/
  public:
    BoxList boxes;
    TopLayerData(int row, int col):y(row),x(col){
      setRunTimeVersion("0.0",0);
      setName("_ij_");
    }
    TopLayerData(int row, int col, int part, bool):y(row),x(col),part_no(part){
      setRunTimeVersion("0.0",0);
      setName("_ijk_");
      cout <<"22\n"; 
    }
    int get_row(){return y;}
    int get_col(){return x;}
    int part_count(){return parts.size();}
    void make_groups();
    void make_partitions();
    /*---------------------------------------------------*/

    TopLayerData(int M_, int N_,int np_):ng(N_),np(np_){
      IData::M=M_;
      IData::N=N_;
      for(int j=0;j<N;j++){
	      for(int i=0;i<M;i++){
             TopLayerData *g = new TopLayerData(i,j);
	        groups.push_back(g);
                 for( int k=0;k<np;k++){
                  g->parts.push_back(new GData (i,j,k,true));
                 }
        }
      }      

      //make_groups();
      //make_partitions();
    }
    /*---------------------------------------------------*/
    TopLayerData &operator()(int i,int j){
      return *groups[j*M+i];
    }
    /*---------------------------------------------------*/
    bool contains(Box *b){
      for( Box *my_b: boxes){
	if (my_b->index == b->index)
	  return true;
      }
      return false;
    }
    /*---------------------------------------------------*/
    TopLayerData &operator[](int i){
      int n = parts.size();
      (void)n;
      return *parts[i];
    }
    /*---------------------------------------------------*/
    void show_hierarchy(){
      for (int L=0;L<L_max;L++ ){
	for (int gi=0;gi<Parameters.groups;gi++){
	  TopLayerData &D=(*this)(L,gi);
	  cout << "Boxes in D("<< L << "," << gi<<")\n" ;
	  for(Box *b:D.boxes){
	    D.show_box(b);
	  }
	  for (uint32_t pi=0;pi<D.parts.size();pi++ ){
	    cout << "Boxes in D("<< L << "," << gi<<"," << pi << ")\n" ;
	    for( Box *bp:D[pi].boxes){
	      D[pi].show_box(bp);
	    }
	  }
	}
      }
    }
    /*---------------------------------------------------*/
    void show_box(Box *b){
      cout << "B("<< b->level << "," << b->index << ")" << endl;
    }
    /*---------------------------------------------------*/
    void dump();
    void fill(double v);
    bool check(double v);
    void fill_sequence(double start, double inc);
    bool check_sequence(double start, double inc);

  };
  typedef TopLayerData GData;
  /*---------------------------------------------------*/
  extern FMM_3D::GData *mainF,*mainG;
  
}//namespace FMM_3D
#endif
