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
  bool dlb;
  Config(){}
  void setParams( int n,int m , 
		  int ynb , int xnb ,
		  int P_, int p_, int q_,
		  int mb_, int nb_, 
		  int nt_,
		  bool dlb_,int ipn_){
    N=n;M=m;
    Nb=xnb;Mb=ynb;
    P=P_;p=p_;q=q_;
    nb=nb_;mb=mb_;
    nt=nt_;
    dlb=dlb_;ipn=ipn_;
  }
  Config ( int n,int m , int nb , int mb , int P_, int p_, int q_,int mb_, int nb_, int nt_,bool dlb_,int ipn_):
    N(n),M(m),Nb(nb),Mb(mb),
    P(P_),p(p_),q(q_),
    nb(nb_),mb(mb_),
    nt(nt_),dlb(dlb_),ipn(ipn_)
  {
    printf("cfg.ipn=%d\n",ipn);
  }
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

static Config::cmd_line_struct cmd[]={
  {Config::P_IDX,"-P","Number of Processors"},
  {Config::N_IDX,"-N",""},
  {Config::M_IDX,"-M",""},
  {Config::NB_IDX,"-NB",""},
  {Config::MB_IDX,"-MB",""},
  {Config::p_IDX,"-p",""},
  {Config::q_IDX,"-q",""},
  {Config::nb_IDX,"-nb",""},
  {Config::mb_IDX,"-mb",""},
  {Config::IPN_IDX,"-IPN",""},
  {Config::DLB_IDX,"-DLB",""},
  {Config::NT_IDX,"-NT",""},
  {Config::BLAS_IDX,"-BLAS",""},
  {-1,"",""}
  };

Config config;
#endif //__CONFIG_HPP_
