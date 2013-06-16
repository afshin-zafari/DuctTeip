#include "context.hpp"
int main () 
{
  int N=100,
    M=100,
    Nb=4,
    Mb=4,
    P= 6,
    p= 2,
    q= 3;
  int i,j;

  Context MA("MatrixAssembly");
  Context Distance("Distance");
  Context RBF("RBF_Phi");
  Data D("Dist",N,N,&MA),R("RBF",N,N,&MA),V("vP",N,1,&MA);

  V.setPartition (Nb,1); 
  D.setPartition(Nb,Nb);
  R.setPartition(Nb,Nb);


  Distance.setParent (&MA);
  RBF.setParent (&MA);

  Distance.addInputData (&V);
  Distance.addOutputData(&D);
  RBF.addInputData  (&D);
  RBF.addOutputData (&R);
  
  Context *col;
  ProcessGrid PG(P,p,q);
  DataHostPolicy dhpMA(&PG) ;
  MA.setDataHostPolicy(&dhpMA);

  char task_name[20];
  col = new Context[Nb];
  for (  me =0; me<P; me++ ) {
    cout << "\n---- Node " << me << "----" << endl;
    for ( i=0; i< Nb ; i++){
      for ( j=0;j<Nb;j++){
	if (PG.isInList(me, PG.col(j)) ){
	  sprintf(task_name,"DIST_%2.2d_%2.2d",i,j);
	  AddTask(task_name,V(i),V(j),D(i,j) ) ;
	}
      }
    }
    for ( i=0; i< Nb ; i++){
      if (PG.isInList(me, PG.row(i)) ){
	for ( j=0;j<Nb;j++){
	  sprintf(task_name,"RBF_%2.2d_%2.2d",i,j);
	  AddTask(task_name,D(i,j),NULL,R(i,j) ) ;
	}
      }
    }
  }

}
