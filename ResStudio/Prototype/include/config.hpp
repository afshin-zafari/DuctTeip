#ifndef __CONFIG_HPP_ 
#define __CONFIG_HPP_

#define GETTER(a,b) int get##a(){return b;}
#define SETTER(a,b) void set##a(int _p){b=_p;}
#define PROPERTY(a,b) GETTER(a,b) \
  SETTER(a,b)

class Config
{
private:
public:
  enum {
    P_IDX=0,
    N_IDX,M_IDX,
    p_IDX,q_IDX,
    NB_IDX,MB_IDX,
    NT_IDX,
    nb_IDX,mb_IDX,
    IPN_IDX,DLB_IDX,
    BLAS_IDX,
    PARAMS_COUNT
  };
  typedef struct {
    int idx; char *flag,*help;
  }cmd_line_struct;
  static cmd_line_struct cmd[];
  int params[PARAMS_COUNT];
  int N,M,Nb,Mb,P,p,q,mb,nb,nt,ipn;
  bool dlb,using_blas,row_major,column_major;
  Config();
  void setParams( int n,int m , 
		  int ynb , int xnb ,
		  int P_, int p_, int q_,
		  int mb_, int nb_, 
		  int nt_,
		  bool dlb_,int ipn_,bool blas = true);
  Config ( int n,int m , int nb , int mb , int P_, int p_, int q_,int mb_, int nb_, int nt_,bool dlb_,int ipn_);
  PROPERTY(XDimension,N)
  PROPERTY(YDimension,M)
  PROPERTY(XBlocks,Nb)
  PROPERTY(YBlocks,Mb)
  PROPERTY(XLocalBlocks,nb)
  PROPERTY(YLocalBlocks,mb)
  PROPERTY(Processors,P)
  PROPERTY(P_pxq,p)
  PROPERTY(Q_pxq,q)
  PROPERTY(NumThreads,nt)
  PROPERTY(DLB,dlb)
  PROPERTY(IPN,ipn)

};

extern Config config;
#endif //__CONFIG_HPP_
