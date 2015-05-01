bool check_gemm(double *d,int M, int N,double key){
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      if ((key-M+j) != d[i+j*M] ) {
	printf("g[%ld]# key:%lf\n",pthread_self(),key);
	return false;
      }
    }
  }
  return true;
}


bool check_syrk(double *d,int M, int N,double key){
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      if ( i == j ) {
	if ((key-M+j) != d[i+j*M]){
	  printf("s[%ld]# key:%lf\n",pthread_self(),key);
	  return false;
	}
      }
      else{
	if ( i > j ) {
	  if ((key-M-2+j) != d[i+j*M] ) {
	    printf("s[%ld]# key:%lf\n",pthread_self(),key);
	    return false;
	  }
	}
      }
    }
  }
  return true;
}

bool check_trsm(double *d,int M, int N){
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      if (d[i+j*M] != -1 ) {
	printf("t[%ld]# key:%lf\n",pthread_self(),d[i+j*M]);
	return false;
      }
    }
  }
  return true;
}
bool check_potrf(double *d,int M, int N){
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      if (i == j ){
	if (d[i+j*M] != 1 ){
	  printf("s[%ld]# key:%lf\n",pthread_self(),d[i+j*M]);
	  return false;
	}
      }
      else{
	if ( i>j){
	  if (d[i+j*M] != -1 ) {
	    printf("p[%ld]# key:%lf\n",pthread_self(),d[i+j*M]);
	    return false;
	  }
	}
      }
    }
  }
  return true;
}
void dumpData(double *d,int M,int N,char t=' '){
  return;
  if (0 && !DEBUG_DLB_DEEP)
    return;
  if ( 0 && DUMP_FLAG)
    return ;
  char s[1000]="";
  printf("[%ld]* %c: %p %ld %p\n",pthread_self(),t,d,M*M*sizeof(double),d+M*M);
  for ( int i = 0 ; i < M; i ++){
    for ( int j =0 ; j< M; j++){
      sprintf (s+j*5," %3.0lf ",d[i+j*N]);
    }
    printf("[%ld] : %s\n",pthread_self(),s);
  }
  printf("[%ld] : ----------------------------------------\n",pthread_self());
}
