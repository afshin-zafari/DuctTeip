#include "mpi.h"
int main ( int argc , char *argv[]) 
{
  int me,dest,p;
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  MPI_Comm_size(MPI_COMM_WORLD,&p);
  MPI_Request *req= new  MPI_Request[(p-1)*10];
  int v ; 
  for ( int i = 0 ; i< 10 ; i++){
    for ( dest = 0; dest < p; dest ++){
      v = me * 1000 + dest * 100 + i ;
      if ( dest != me ) {
	MPI_Isend(&v,1,MPI_INT,dest,1,MPI_COMM_WORLD,&req[i]);
	printf("send %d from %d to %d.\n",v,me,dest);
      }
    }
  }
  int flag,recv_count=0 ,r;
  
  MPI_Status status;
  while ( recv_count < (p-1)*10 ) {
    MPI_Iprobe(MPI_ANY_SOURCE,1,MPI_COMM_WORLD,&flag,&status);
    if ( flag ) {
      recv_count++;
      MPI_Recv(&r,1,MPI_INT,status.MPI_SOURCE,1,MPI_COMM_WORLD,&status);
      printf("proc %d recv %d from %d .\n",me,r,status.MPI_SOURCE);
    }
  }
  
  MPI_Finalize();
  
  
}
