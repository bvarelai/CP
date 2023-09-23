    #include <stdio.h>
    #include <stdlib.h>
    #include <math.h>
    #include <mpi.h> 

    //send es pi
    //recv es aux

    int MPI_FlattreeColectiva(double *sendbuf, double *recvbuf, int count,MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
    {
        int error,k;
        MPI_Status status;
        int rank,numprocs;

        MPI_Comm_size(MPI_COMM_WORLD,&numprocs); //Para que los procesos sepan el numero de procesos 
        MPI_Comm_rank(MPI_COMM_WORLD,&rank); //Saber el ID de cada uno

        if(rank>root)
        {  
          
           MPI_Send(sendbuf,count,datatype,0,0,comm); //Los procesos envian el valor de pi al proceso 0.            
           if(error!=MPI_SUCCESS) return error;   
        }
        else
        {
          for(k=1;k<numprocs;k++)
          {
              error=MPI_Recv(recvbuf,count,datatype,k,0,comm,&status); //El root envia el valor al resto de procesos
              *recvbuf+=(numprocs-1)*(*sendbuf);
              if(error!=MPI_SUCCESS) return error;  
          }
        }
    }

    int MPI_BinomialBcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
    {
      int error, k;
      int numprocs, rank;
      int receiver, sender;

      MPI_Status status;

      MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);

      for (k = 1; k <= ceil(log2(numprocs)); k++){
        if (rank < pow(2, k-1))   //con k=3 llegaria hasta 3
        {
            receiver = rank + pow(2, k-1);
            error = MPI_Send(buf, count, datatype, receiver, 0, comm);
              if (error != MPI_SUCCESS) return error;
        } else
        {
            if (rank < pow(2, k))  // con k=3 llegaria hasta 7
            {
              sender = rank - pow(2, k-1);
              error = MPI_Recv(buf, count, datatype, sender, 0, comm, &status);
              if (error != MPI_SUCCESS) return error;
            }
        }
      }
    }

    int main(int argc, char *argv[])
    {
        int i, done = 0, n, count,k;
        double PI25DT = 3.141592653589793238462643;
        double pi, x, y, z,aux;
        int numprocs,rank; 

        MPI_Init(&argc,&argv); //Sincronizacion de los procesos.
        MPI_Comm_size(MPI_COMM_WORLD,&numprocs); //Para que los procesos sepan el numero de procesos 
        MPI_Comm_rank(MPI_COMM_WORLD,&rank); //Saber el ID de cada uno

        while(!done)
        {
           if(rank==0)
           {
            
             printf("Enter the number of points: (0 quits) \n");
             scanf("%d",&n);
           }
             
          // MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
           MPI_BinomialBcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);      
           
           if (n == 0) break;
            
           count = 0;  
            
           for (i = 0; i <= n; i+=numprocs) 
           {
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
  
           //MPI_Reduce(&pi,&aux,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
           MPI_FlattreeColectiva(&pi,&aux,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);       
           

            if(rank==0)
              printf("pi is approx. %.16f, Error is %.16f\n", aux, fabs(aux - PI25DT));      
        }
        MPI_Finalize();
    }
 
