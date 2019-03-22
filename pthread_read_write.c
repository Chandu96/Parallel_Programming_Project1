/* header files*/
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/time.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>

void *find_min_rw(void *list_ptr);

int minimum_value, partial_list_size;


typedef struct{
    int readers;
    int writer;
    pthread_cond_t readers_proceed;
    pthread_cond_t writer_proceed;
    int pending_writers;
    pthread_mutex_t read_write_lock;
} mylib_rwlock_t;


static double mysecond()
{
    struct timeval  tp;
    struct timezone tzp;
    int i = 0;
    i = gettimeofday(&tp, &tzp);
    return ((double)tp.tv_sec + (double)tp.tv_usec * 1.e-6);
}

void mylib_rwlock_init(mylib_rwlock_t *l)
{
    l -> readers = l -> writer = l ->pending_writers =0;
    pthread_mutex_init(&(l -> read_write_lock), NULL);
    pthread_cond_init(&(l -> readers_proceed), NULL);
    pthread_cond_init(&(l -> writer_proceed), NULL);
}

void mylib_rwlock_rlock(mylib_rwlock_t *l)
{
    pthread_mutex_lock(&(l -> read_write_lock));
    while((l -> pending_writers >0) || (l -> writer >0))
        pthread_cond_wait(&(l -> readers_proceed), &(l -> read_write_lock));
    l -> readers ++;
    pthread_mutex_unlock(&(l -> read_write_lock));
}

void mylib_rwlock_wlock(mylib_rwlock_t *l)
{
    pthread_mutex_lock(&(l -> read_write_lock));
    while((l -> writer >0) || (l -> readers >0))
    {
        l -> pending_writers++;
        pthread_cond_wait(&(l -> writer_proceed),
                          &(l -> read_write_lock));
    }
    l -> pending_writers --;
    l -> writer ++;
    pthread_mutex_unlock(&(l -> read_write_lock));
}

void mylib_rwlock_unlock(mylib_rwlock_t *l)
{
    pthread_mutex_lock(&(l -> read_write_lock));
    if(l -> writer >0)
        l -> writer = 0;
    else if(l -> readers >0)
        l -> readers --;
    pthread_mutex_unlock(&(l -> read_write_lock));
    if((l -> readers == 0) && (l -> pending_writers >0))
        pthread_cond_signal(& (l -> writer_proceed));
    else if(l -> readers >0)
        pthread_cond_broadcast(&(l -> readers_proceed));
}

/* main function begins here*/
int main(int argc, char **argv) {

    int num_of_threads = 1;
    while (num_of_threads <= 8){
        int ret = 0;
    long num_of_elements = 100000000;
    int result;
    double start = 0;
    double end = 0;
    int seed = 4;
    pthread_t *threads = NULL;
    int *list = NULL;
    long cur = 0;

    /* sanity check */
    if (num_of_threads < 1) {
        printf("error : not enough threads\n");
        return -1;
    }
    if (num_of_elements <= (long) (num_of_threads)) {
        printf("error : not enough elements\n");
        return -1;
    }
    minimum_value = INT_MAX;
    list = malloc(sizeof(int) * num_of_elements);
    if (list == NULL) {
        printf("Error : could not init the list\n");
        return -1;
    }
    threads = malloc(sizeof(pthread_t) * num_of_threads);
    if (threads == NULL) {
        printf("Error : could not init the tids\n");
        return -1;
    }
    srand(seed);
    for (int l = 0; l < num_of_elements; l++) {
        list[l] = (long) (rand());
    }

    if (num_of_threads == 1) {
        partial_list_size = num_of_elements;
    } else {
        partial_list_size = (num_of_elements / (long) (num_of_threads)) + (num_of_elements % (long) (num_of_threads));
    }
    start = mysecond();
    for (int i = 0; i < num_of_threads; i++) {

        result = pthread_create(&threads[i], NULL, &find_min_rw, &list[cur]);
        cur += partial_list_size;
        if ((cur + partial_list_size) > num_of_elements) {
            cur = num_of_elements - partial_list_size;
        }
    }
    if (result != 0) {
        printf("ERROR; return code from pthread_create() is %d\n", result);
        exit(-1);
    }

    for (int i = 0; i < num_of_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            printf("Error : pthread_join failed on joining thread %d\n", i);
            return -1;
        }

    }

    end = mysecond();

    printf("\n\nthis is final minimum value  %d\n", minimum_value);
    printf("\nRuntime of %d threads = %f seconds for %ld elements\n", num_of_threads, (end - start), num_of_elements);


    free(list);
    free(threads);

    num_of_threads = num_of_threads * 2;
    }

}
void *find_min_rw(void *list_ptr)
{
    int *partial_list_pointer, my_min, i;


    mylib_rwlock_t read_write_lock;

    mylib_rwlock_init(&read_write_lock);

    partial_list_pointer= (int *)list_ptr;
    my_min=partial_list_pointer[0];
    for(i=0; i<partial_list_size; i++)
    {
        if(partial_list_pointer[i] < my_min)
        {
            my_min=partial_list_pointer[i];
        }
    }

    mylib_rwlock_rlock(&read_write_lock);
    if(my_min<minimum_value)
    {
        mylib_rwlock_unlock(&read_write_lock);
        mylib_rwlock_wlock(&read_write_lock);
        minimum_value=my_min;
    }
    mylib_rwlock_unlock(&read_write_lock);
    pthread_exit(0);
}

