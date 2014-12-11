#ifndef __CONFIG_HPP_
#define __CONFIG_HPP_

#include <stdlib.h>
#include <cstring>
#include <iostream>

#define GETTER(a,b) int get##a(){return b;}
#define SETTER(a,b) void set##a(int _p){b=_p;}
#define PROPERTY(a,b) GETTER(a,b) \
  SETTER(a,b)

// environment settings
class Config
{
private:
  int N,M,Nb,Mb,P,p,q,mb,nb,nt;
  bool dlb,simulation;
  static const int INVALID_NUMBER=-1;
public:
  Config (int *argc,char **argv){
    N=M=Nb=Mb=P=p=q=mb=nb=nt=INVALID_NUMBER;
    for ( int i=1;i<*argc-1;i++){
      if ( !strcmp(argv[i],"-P"))P=atoi(argv[++i]); 
      if ( !strcmp(argv[i],"-p"))p=atoi(argv[++i]); 
      if ( !strcmp(argv[i],"-q"))q=atoi(argv[++i]); 
      if ( !strcmp(argv[i],"-M"))M=atoi(argv[++i]); 
      if ( !strcmp(argv[i],"-N"))N=atoi(argv[++i]); 
      if ( !strcmp(argv[i],"-B"))Mb=atoi(argv[++i]); 
      if ( !strcmp(argv[i],"-b"))mb=atoi(argv[++i]); 
      if ( !strcmp(argv[i],"-t"))nt=atoi(argv[++i]); 
      if ( !strcmp(argv[i],"-D"))dlb=(atoi(argv[++i])!=0); 
      if ( !strcmp(argv[i],"-S"))simulation=(atoi(argv[++i])!=0); 
    }
    if ( !isPositiveInt(P)){
      std::cerr << "Not complete params.\n";
      printf("Usage :\n");
      exit(-1);
    }
    
    if ( isPositiveInt(N)){
      if ( !isPositiveInt(M) ) {
	M=N;
      }
    }
    
    
  }
  bool isPositiveInt(int a){
    if ( a != INVALID_NUMBER)
      if ( a >0)
	return true;
    return false;
  }
  Config ( int n,int m , int nb , int mb , int P_, int p_, int q_,int mb_, int nb_, int nt_,bool dlb_):
    N(n),M(m),Nb(nb),Mb(mb),P(P_),p(p_),q(q_),nb(nb_),mb(mb_),nt(nt_),dlb(dlb_){}
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

};
#endif //__CONFIG_HPP_
