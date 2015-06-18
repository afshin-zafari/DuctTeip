#include "config.hpp"

  Config::Config(){}
  void Config::setParams( int n,int m , 
		  int ynb , int xnb ,
		  int P_, int p_, int q_,
		  int mb_, int nb_, 
		  int nt_,
		  bool dlb_,int ipn_,bool blas ){
    N=n;M=m;
    Nb=xnb;Mb=ynb;
    P=P_;p=p_;q=q_;
    nb=nb_;mb=mb_;
    nt=nt_;
    dlb=dlb_;ipn=ipn_;
    using_blas = blas;
    column_major = using_blas;
    row_major = !column_major;
  }
  Config::Config ( int n,int m , int nb , int mb , int P_, int p_, int q_,int mb_, int nb_, int nt_,bool dlb_,int ipn_):
    N(n),M(m),Nb(nb),Mb(mb),
    P(P_),p(p_),q(q_),
    nb(nb_),mb(mb_),
    nt(nt_),dlb(dlb_),ipn(ipn_)
  {
#ifdef BLAS
    using_blas = true;
    column_major = true;
    row_major  false;
#else
    using_blas = false;
#endif
  }

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


