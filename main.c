#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS 5

struct thread_data {
   int thread_id;
   int x;
   int y;
};

void *ThreadFunc(void *threadid);


int main(int argc, char **argv) {
   pthread_t threads[NUM_THREADS];
   int returnVal;
   long ndx;
   struct thread_data arr[NUM_THREADS];   
   
   for (ndx = 0; ndx < NUM_THREADS; ndx++) {
      printf("Creating Threads\n"); 
      arr[ndx].thread_id = ndx;
      arr[ndx].x = ndx;
      arr[ndx].y = 70;

      returnVal = pthread_create(&threads[ndx], NULL, ThreadFunc, (void *) &arr[ndx]);

      if (returnVal) {
         printf("ERROR: %d\n", returnVal);
         exit(-1);
      }
   } 
   pthread_exit(NULL);
}

void *ThreadFunc(void *threadarg) {
   int tid, x, y;
   struct thread_data *my_data = (struct thread_data *) threadarg;

   tid = my_data->thread_id;
   x = my_data->x;
   y = my_data->y;

   printf("Childs val %d\n", x + y);
   pthread_exit(NULL);
}

