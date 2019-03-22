CC:=gcc

program: pthread_read_write.c pthreads_findmin.c pthreads_gauss.c

	 $(CC) pthread_read_write.c -lpthread -o read_write.out

	 $(CC) pthreads_findmin.c -lpthread -o findmin.out

	 $(CC) pthreads_gauss.c -lpthread -o gauss.out
	 


clean:
	rm -rf *.out
