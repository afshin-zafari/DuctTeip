#include "config.hpp"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>


Config config;
/*-----------------------------------------------------------------*/
Config::Config(){
  P=p=q=N=M=Mb=nb=Nb=nb=nt=ipn=to=ps=-1;
  dlb=false;
  column_major=using_blas=true;
}
/*-----------------------------------------------------------------*/
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
/*-----------------------------------------------------------------*/
Config::Config ( int n,int m , int nb , int mb , int P_, int p_,
		 int q_,int mb_, int nb_, int nt_,bool dlb_,int ipn_):
  N(n),M(m),Nb(nb),Mb(mb),
  P(P_),p(p_),q(q_),
  nb(nb_),mb(mb_),
  nt(nt_),dlb(dlb_),ipn(ipn_){}

/*-----------------------------------------------------------------*/
void Config::getCmdLine(int argc, char **argv){
  
  static struct option opts[]=
    {
      {"dyn-load-bal" ,no_argument      , 0,'D'}, // 0 
      {"dlb"          ,no_argument      , 0,'D'},
      {"BLAS"         ,no_argument      , 0,'L'}, // 2
      {"blas"         ,no_argument      , 0,'L'},
      {"num-proc"     ,required_argument, 0,'P'}, // 4
      {"proc-grid-row",required_argument, 0,'p'},
      {"proc-grid-col",required_argument, 0,'q'}, // 6
      {"data-rows"    ,required_argument, 0,'N'},
      {"data-cols"    ,required_argument, 0,'M'}, // 8
      {"num-threads"  ,required_argument, 0,'t'},
      {"inst-per-node",required_argument, 0,'I'}, // 10
      {"ipn"          ,required_argument, 0,'I'},
      {"timeout"      ,required_argument, 0,'T'}, // 12
      {"poll-sleep"   ,required_argument, 0,'S'}, // 
      {"simulation"   ,no_argument      , 0,'U'}, // 14
      {0,0,0,0}
    };
  int index;
  while (true){
    int ret=getopt_long(argc,argv,"DLP:p:q:N:M:B:b:t:I:T:S:",opts,&index);
    if ( ret ==-1)
      break;
    switch (ret){
    case 0:
      /*-------------------Long Options ----------------*/
      switch(index)
	{
	case 0:
	case 1:
	  dlb = true;
	  break;
	case 2:
	case 3:
	  using_blas=true;
	  break;
	case 14: 
	  simulation = true;
	  break;
	case 4: P = atoi(optarg);break;
	case 5: p = atoi(optarg);break;
	case 6: q = atoi(optarg);break;
	case 9: nt= atoi(optarg);break;
	case 10: 
	case 11: ipn = atoi(optarg);break;
	case 12: to  = atoi(optarg);break;
	case 13: ps  = atoi(optarg);break;
	case 7:
	  N=atoi(optarg);
	  Nb=atoi(argv[optind]);
	  nb=atoi(argv[optind+1]);
	  break;
	case 8:
	  M=atoi(optarg);
	  Mb=atoi(argv[optind]);
	  mb=atoi(argv[optind+1]);
	  break;
	default:
	  break;	
	}
      /*------------------------------------------------*/
    case 'D':
      dlb=true;
      break;
    case 'L':
      using_blas = true;
      break;
    case 'P':
      P = atoi(optarg);
      break;
    case 'p':
      p=atoi(optarg);
      break;
    case 'q':
      q=atoi(optarg);
      break;      
    case 'N':
      N=atoi(optarg);
      Nb=atoi(argv[optind]);
      nb=atoi(argv[optind+1]);
      opterr=0;
      break;
    case 'M':
      M=atoi(optarg);
      Mb=atoi(argv[optind]);
      mb=atoi(argv[optind+1]);
      opterr=0;
      break;
    case 'B':      
    case 'b':
      break;
    case 't':
      nt=atoi(optarg);
      break;
    case 'I':
      ipn=atoi(optarg);
      break;
    case 'T':
      to=atoi(optarg);
      break;
    case 'S':
      ps=atoi(optarg);
      break;
    case '?':
    default:
      break;      
      
    }
  }
  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
  }
  int err=0;
  if ( M<=0){
    if ( N>0)
      M=N;
    else {
      err =1;
      fprintf(stderr,"Either N or M must be positive non-zero\n");
    }
  }
  if ( N<=0){
    if ( M>0)
      N=M;
    else {
      err =1;
      fprintf(stderr,"Either N or M must be positive non-zero\n");
    }
  }
  if ( Nb<=0){
    if ( Mb>0)
      Nb=Mb;
    else {
      err =2;
      fprintf(stderr,"Either Nb or Mb must be positive non-zero\n");
    }
  }
  if ( Mb<=0){
    if ( Nb>0)
      Mb=Nb;
    else {
      err =2;
      fprintf(stderr,"Either Nb or Mb must be positive non-zero\n");
    }
  }
  if ( nb<=0){
    if ( mb>0)
      nb=mb;
    else {
      err =3;
      fprintf(stderr,"Either nb or mb must be positive non-zero\n");
    }
  }
  if ( mb<=0){
    if ( nb>0)
      mb=nb;
    else {
      err =3;
      fprintf(stderr,"Either nb or mb must be positive non-zero\n");
    }
  }
  if(ipn<=0)
    ipn =1;
  if(nt<=0){
    err=4;
    fprintf(stderr,"nt must be positive non-zero.\n");    
  }
  if(P <=0){
    err=5;
    fprintf(stderr,"P must be positive non-zero.\n");    
  }
  if(p <=0){
    err=5;
    fprintf(stderr,"p must be positive non-zero.\n");    
  }
  if(q <=0){
    err=5;
    fprintf(stderr,"q must be positive non-zero.\n");    
  }
  if(to<=0)
    to=3;
  if(!err){
    if ( Mb < p ){
      err=6;
      fprintf(stderr,"Mb  must be >= p .\n");
    }
    if ( Nb < q ){
      err=6;
      fprintf(stderr,"Nb  must be >= q .\n");
    }
  }
  if ( ps<=0)
    ps =10000;
  LOG_INFO(LOG_CONFIG,"ipn:%d, P:%d,p:%d,q:%d\n",ipn,P,p,q);
  LOG_INFO(LOG_CONFIG,"N:%d, Nb:%d,nb:%d,nt:%d dlb:%d\n",N,Nb,nb,nt,dlb);
  LOG_INFO(LOG_CONFIG,"M:%d, Mb:%d,mb:%d,blas:%d ps:%d\n",M,Mb,mb,using_blas,ps);
  if(err)
    exit(err);
      
}
