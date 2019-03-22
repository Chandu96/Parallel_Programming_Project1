#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>




#define MAXN 2000 /*Maximum Dimension supported*/

#define MAX_PRINT 10 /*Maximum matrix size that can be printed*/

int N = 1000; /*Matrix Size*/

/*for solving A * X = B equations*/
volatile float A[MAXN][MAXN], B[MAXN], X[MAXN];

int num_threads;/*Number Of Threads Used*/
int norm, current_row, count;
#define MAX_THREADS 500
/*Initializing Thread Array*/
pthread_t Threads[MAX_THREADS];

#define min(a, b) ((a) < (b)) ? (a) : (b)  /* finds minimum value*/



void *gauss(void *);


//void parameters(int , char **); /*Set The Program Parameters From The Command-Line Arguments*/
void initialise_inputs();
void print_inputs();
void print_X();

int main()
{
    /*Elapsed times using <gettimeofday()>*/
    struct timeval etstart, etstop;
    struct timezone tzdummy;
    clock_t etstartt, etstoptt; /*Elapsed times using <times()>*/
    unsigned long usecstart, usecstop; /*CPU times for the threads*/
    struct tms cputstart, cputstop;
    int i, row, col;
    //parameters(argc, argv);
    printf("\nENTER NUMBER OF THREADS TO BE EXECUTED:\n");
    scanf("%d", &num_threads);
    initialise_inputs();
    print_inputs();
    current_row = norm + 1;
    count = num_threads - 1;
    //while (num_threads <= 8)


        //printf("Starting clock ...\n");
        gettimeofday(&etstart, &tzdummy);
        etstartt = times(&cputstart);
        for (i = 0; i < num_threads; i++)
        {
            pthread_create(&Threads[i], NULL, gauss, NULL);
        }
        for (i = 0; i < num_threads; i++)
        {
            pthread_join(Threads[i], NULL);
        }

    for (row = N - 1; row >= 0; row--) {
        X[row] = B[row];
        for (col = N - 1; col > row; col--)
            X[row] = X[row] - A[row][col] * X[col];
        X[row] = X[row] / A[row][row];
    }

        gettimeofday(&etstop, &tzdummy);
        etstoptt = times(&cputstop);
        //printf("Stopped clock.\n");
        usecstart = (unsigned long) etstart.tv_sec * 1000000 + etstart.tv_usec;
        usecstop = (unsigned long) etstop.tv_sec * 1000000 + etstop.tv_usec;
        printf("Elapsed time = %g ms.\n", (float) (usecstop - usecstart) / (float) 1000);
        printf("Elapsed time according to <times()> = %g ms.\n", (etstoptt - etstartt) / (float) _SC_CLK_TCK * 1000);
        printf("CPU times are accurate to the nearest %g ms.\n", 1.0 / (float) _SC_CLK_TCK * 1000.0);
        printf("The total CPU time for parent = %g ms.\n", (float) ((cputstop.tms_utime + cputstop.tms_stime) - (cputstart.tms_utime + cputstart.tms_stime)) / (float) _SC_CLK_TCK * 1000);
        printf("The system CPU time for parent = %g ms.\n", (float) (cputstop.tms_stime - cputstart.tms_stime) / (float) _SC_CLK_TCK * 1000);


    printf("\nRESULT:");
    print_X();
}



/*Initializing Inputs for A, B and X*/
void initialise_inputs()
{
    int row, col;
    printf("\nInitialising Inputs...\n");
    for (col = 0; col < N; col++)
    {
        for (row = 0; row < N; row++)
        {
            A[row][col] = (float)rand()/RAND_MAX+1;
        }
        B[col] = (float)rand()/RAND_MAX+1;
        X[col] = 0;
    }
}

/*Print Inputs until the MAX_PRINT size is reached */
void print_inputs()
{
    int row, col;
    if (N < MAX_PRINT)
    {
        printf("\nA =\n\t");
        for (row = 0; row < N; row++)
        {
            for (col = 0; col < N; col++)
                printf("%6.3f  %s", A[row][col], (col < N-1) ? ", " : ";\n\t");
        }
        printf("\nB = [");
        for (col = 0; col < N; col++)
            printf("%6.3f  %s", B[col], (col < N-1) ? "; " : "]\n");
    }

}

/* Printing the X value results */
void print_X()
{
    int row;
    if (N < MAX_PRINT)
    {
        printf("\nX = [");
        for (row = 0; row < N; row++)
            printf("%6.3f  %s", X[row], (row < N-1) ? "; " : "]\n");
    }
}

/* PARALLELIZED FROM HERE */


/* Initialization of Mutex Initializer */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*get_chunk_size() Function Is Used For Determining The Chunk Size
 *Which Can Be Used For Guided-Self Scheduling*/
int partial_row_size(int *, int );

void barrier(int * );
/*This is used to split the entire row index into set of chunks*/
int partial_row_size(int *my_row, int Norm)
{
    int partial_size;
    int split_par = (N-Norm-1)/(2*num_threads)+1; /*Splitting row*/
    pthread_mutex_lock(&mutex); /* Critical Section Starts*/
    *my_row = current_row;
    int val = (*my_row < N);
    /*Determining Chunk Size*/
    if(val){
        partial_size = split_par; /*If there is still a need for row to be computed*/
    }
    else{
        partial_size = 0; /* if no more row chunks are available*/
    }
    current_row = current_row + partial_size; /*Update row value for computation*/
    pthread_mutex_unlock(&mutex); /* Critical Section Ends */
    return partial_size;
}

/* GAUSSIAN ELIMINATION */
void *gauss(void* NIL)
{

    int my_row = 0, row, col;
    int Norm = 0;
    float multiplier;
    int partial_size;

    /*Gaussian Elimination*/
    while (Norm < N-1)
    {
        /*Dividing the row value into multiple parts*/
        while (partial_size = partial_row_size(&my_row, Norm))
        {
            for (row = my_row; row < (min(N, my_row+partial_size)); row++)
            {
                multiplier = A[row][Norm]/A[Norm][Norm];
                for (col = Norm; col < N; col++)
                {
                    A[row][col] -= A[Norm][col]*multiplier;
                }
                B[row] -= B[Norm]*multiplier;
            }
        }
        /*Incrementing the Value of Norm*/
        barrier(&Norm); /*Synchronization of Threads to access norm values*/
    }


}
void broadcast_inc();/* Used before broadcasting */
void broadcast_inc(){
    current_row = norm+1;
    count = num_threads-1;
    norm++;
}

pthread_mutex_t counter = PTHREAD_MUTEX_INITIALIZER; /*MUTEX LOCK FOR CONDITIONAL VARIABLE */
pthread_cond_t conditional = PTHREAD_COND_INITIALIZER; /* for conditional variables to be used */

/*In barrier we make all threads wait to pick up the value of norm, where the control varies between the conditional sections*/
void barrier(int *Norm)
{
    /*Implementing Synchronisation Using Condition Variables */
    pthread_mutex_lock(&counter); /* Critical Section Starts */
    if (count == 0)
    {
        /* Increments norm and new thread enters and tries to access this section */
        broadcast_inc();
        pthread_cond_broadcast(&conditional);
    }
    else
    {
        count--;
        pthread_cond_wait(&conditional, &counter); /*Makes thread wait until the broadcast_inc() section is reached*/
    }
    /* Unlocking the mutex the place where critical section ends */
    *Norm = norm;
    pthread_mutex_unlock(&counter); /*Critical Section Ends*/
}






