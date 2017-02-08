#ifndef __CONFIG_HPP_
#define __CONFIG_HPP_
//#include "basic.hpp"
#include <string>

#define GETTER(a,b) int get##a(){return b;}
#define SETTER(a,b) void set##a(int _p){b=_p;}
#define PROPERTY(a,b) GETTER(a,b) \
  SETTER(a,b)

class Config
{
private:
public:
  int N,M;
  short int Nb,Mb,P,p,q,mb,nb,nt,ipn,to,ps,sil_dur,dlb_thr;
  bool dlb,using_blas,row_major,column_major,simulation,dlb_smart,mq_mode;
  std::string sch1,sch2,sch3,lib1,lib2,lib3;
  std::string mq_send,mq_recv,mq_ip,mq_name,mq_pass;
  Config();
  void setParams( int n,int m ,
		  int ynb , int xnb ,
		  int P_, int p_, int q_,
		  int mb_, int nb_,
		  int nt_,
		  bool dlb_,int ipn_,bool blas = true);
  Config ( int n,int m , int nb , int mb , int P_, int p_, int q_,int mb_, int nb_, int nt_,bool dlb_,int ipn_);
  void getCmdLine(int, char **);
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
  PROPERTY(SilenceDuration,sil_dur)
  PROPERTY(DLBThreshold,dlb_thr)
  PROPERTY(DLBSmart,dlb_smart)

};

extern Config config;
#endif //__CONFIG_HPP_
