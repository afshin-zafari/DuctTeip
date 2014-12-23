#ifndef __CONFIG_HPP_
#define __CONFIG_HPP_

#define GETTER(a,b) int get##a(){return b;}
#define SETTER(a,b) void set##a(int _p){b=_p;}
#define PROPERTY(a,b) GETTER(a,b) \
  SETTER(a,b)

// environment settings
class Config
{
private:
  int N,M,Nb,Mb,P,p,q,mb,nb,nt,ipn;
  bool dlb;
public:
  Config ( int n,int m , int nb , int mb , int P_, int p_, int q_,int mb_, int nb_, int nt_,bool dlb_,int ipn_):
    N(n),M(m),Nb(nb),Mb(mb),P(P_),p(p_),q(q_),nb(nb_),mb(mb_),nt(nt_),dlb(dlb_),ipn(ipn_){printf("cfg.ipn=%d\n",ipn);}
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
#endif //__CONFIG_HPP_
