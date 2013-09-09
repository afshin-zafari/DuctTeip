#ifndef __CONFIG_HPP_
#define __CONFIG_HPP_

#define GETTER(a,b) int get##a(){return b;}
#define SETTER(a,b) void set##a(int p){b=p;}
#define PROPERTY(a,b) GETTER(a,b) \
  SETTER(a,b)

class Config
{
private:  
  int N,M,Nb,Mb,P,p,q;
public:
  Config ( int n,int m , int nb , int mb , int PP, int pp, int qq):
    N(n),M(m),Nb(nb),Mb(mb),P(PP),p(pp),q(qq){}
  PROPERTY(XDimension,N)
  PROPERTY(YDimension,M)
  PROPERTY(XBlocks,Nb)
  PROPERTY(YBlocks,Mb)
  PROPERTY(Processors,P)
  PROPERTY(P_pxq,p)
  PROPERTY(Q_pxq,q)
  
};
#endif //__CONFIG_HPP_
