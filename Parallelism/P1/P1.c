#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h> 

int main(int argc, char *argv[])
{
    int i, done = 0, n, count,k;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z,aux;
    int numprocs,rank; 
    MPI_Status status;

    MPI_Init(&argc,&argv); //Sincronizacion de los procesos.
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs); //Para que los procesos sepan el numero de procesos 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); //Saber el ID de cada uno

    while (!done)
    {
       if(rank==0){
        
         printf("Enter the number of points: (0 quits) \n");
         scanf("%d",&n);
         
         for(k=1;k<numprocs;k++){
           MPI_Send(&n,1,MPI_INT,k,0,MPI_COMM_WORLD); //El proceso 0 envia el valor de n a los demas procesos.  
         } 
       }
         
       else
       {
          MPI_Recv(&n,1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status); //Los procesos reciben el valor de n (enviado por el proceso 0)
       } 
       
       if (n == 0) break;
        
       count = 0;  
        
       for (i = 1; i <= n; i+=numprocs) {
   
            // Get the random numbers between 0 and 1
            x = ((double) rand()) / ((double) RAND_MAX);
            y = ((double) rand()) / ((double) RAND_MAX);

           // Calculate the square root of the squares
           z = sqrt((x*x)+(y*y));

          // Check whether z is within the circle
          if(z <= 1.0)
            count++;
        }
        pi = ((double) count/(double) n)*4.0;
       
    
       if(rank>0){
           MPI_Send(&pi,1,MPI_DOUBLE,0,0,MPI_COMM_WORLD); //Los procesos envian el valor de pi al proceso 0.            
       }
       else{
        
           for(k=1;k<numprocs;k++){

              MPI_Recv (&aux,1,MPI_DOUBLE,k,MPI_ANY_TAG, MPI_COMM_WORLD,&status);  //El proceso 0 recibe la aproximacion de pi      
              pi+=aux;
           }
           printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));      
       }
    }
    MPI_Finalize();
}




