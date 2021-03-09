#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include <shprototypes.h>
#include <ctype.h>
#include <prodcons_bb.h>  
#include <stdlib.h>
#include <future.h>
#include <future_prodcons.h>
#include <future_fib.h>
#include <stream.h>


int32 stream_proc(int, char*);
// variable for future_prodcons
int32 head;
int32 tail;
int32 arr_q[5];
// definition of array, semaphores and indices 

// variable for future_fib
int future_fib(int nargs, char *args[]){
  int fib = -1, i;

    fib = atoi(args[2]);

    if (fib > -1) {
      int final_fib;
      int future_flags = FUTURE_SHARED; // TODO - add appropriate future mode here

      // create the array of future pointers
      if ((fibfut = (future_t **)getmem(sizeof(future_t *) * (fib + 1)))
          == (future_t **) SYSERR) {
        printf("getmem failed\n");
        return(SYSERR);
      }

      // get futures for the future array
      for (i=0; i <= fib; i++) {
        if((fibfut[i] = future_alloc(future_flags, sizeof(int), 1)) == (future_t *) SYSERR) {
          printf("future_alloc failed\n");
          return(SYSERR);
        }
      }

      // spawn fib threads and get final value
      for (i = 0; i <= fib; i++) {
         resume(create((void *)ffib, 4096, 20, "ffib", 1, i));
      }   

      future_get(fibfut[fib], (char*) &final_fib);

      for (i=0; i <= fib; i++) {
        future_free(fibfut[i]);
      }

      freemem((char *)fibfut, sizeof(future_t *) * (fib + 1));
      printf("Nth Fibonacci value for N=%d is %d\n", fib, final_fib);
      return(OK);
    }
    return -1; //?? do we need this? missing parathensis
  }

    int future_free_test(int nargs, char *args[])
    {
    future_t *f_exclusive;
    f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);
    printf("future exclsive created\n");
    future_free(f_exclusive);
    printf("future exclsive freed\n");

    future_t *f_shared;
    f_shared = future_alloc(FUTURE_SHARED, sizeof(int), 1);
    printf("future shared created\n");
    future_free(f_shared);
    printf("future shared freed\n");

    return OK;
    }

void future_prodcons(int nargs, char *args[])
{
    // argument handing: you should make sure arguments are either "g" or "s" or a number. 
    // -pc is the 2nd argument, when -pc is passed expected variable number of arguments
    if (nargs <= 2) {
      printf("Syntax: run futest [-pc [g ...] [s VALUE ...]|-f]\n");
      return;
    }

    print_sem = semcreate(1);
    future_t* f_exclusive;
    f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);

    // First, try to iterate through the arguments and make sure they are all valid based on the requirements 
    // (you may assume the argument after "s" there is always a number)
    int i = 2;
    while (i < nargs)
    {
        // write your code here to check the validity of arguments
        if (strcmp(args[i], "s") == 0) {
          //printf("wrong inputs from %s, num: %d\n", args[i], i);
          int x = atoi(args[i+1]);
          if (x == 0) {
              printf("wrong inputs from s\n");
              return;
          }
          i = i + 2;
          continue;
        } else if (strcmp(args[i], "g") == 0 ) {
          // printf("this is %s, num: %d\n", args[i], i);
          i++;
          continue;
        } else { 
   		printf("Syntax: run futest [-pc [g ...] [sxsh $  VALUE ...]|-f]\n");
   	}
        return;
    }

    int num_args = i;  // keeping number of args to create the array
    i = 2; // reseting the index 
    char* val  =  (char *) getmem(num_args); // initializing the array to keep the "s" numbers

    // Iterate again through the arguments and create the following processes based on the passed argument ("g" or "s VALUE")

    while (i < nargs)
    {
      if (strcmp(args[i], "g") == 0){
        char id[10];
        sprintf(id, "fcons%d",i);
        // printf("%s\n", args[i]);
        resume(create(future_cons, 2048, 20, id, 1, f_exclusive));    
      }
      if (strcmp(args[i], "s") == 0){
        i++;
        uint8 number = atoi(args[i]);
        val[i] = number;
        resume(create(future_prod, 2048, 20, "fprod1", 2, f_exclusive, &val[i]));
        sleepms(5);
      }
      i++;
    }
    sleepms(100);
    future_free(f_exclusive);
}


void prodcons_bb(int nargs, char *args[]) {
  //create and initialize semaphores to necessary values
  w_sem = semcreate(5);
  r_sem = semcreate(0);
  mutex = semcreate(1);

  //initialize read and write indices for the queue
  head = 0;
  tail = 0;
  //create producer and consumer processes and put them in ready queue
  int num_prod = atoi(args[1]);
  int num_cons = atoi(args[2]);
  int prod_iter = atoi(args[3]);
  int cons_iter = atoi(args[4]);

  for (int i = 0; i <num_prod; i++) {
      resume( create(producer_bb, 1024, 20, "producer_bb", 2, i, prod_iter));
  }

  for (int i = 0; i <num_cons; i++) {
      resume( create(consumer_bb, 1024, 20, "consumer_bb", 2, i, cons_iter));
  }
  // return;
}


shellcmd xsh_run(int nargs, char *args[]) {

    if ((nargs == 1) ) {
      printf("hello\n"); 
      printf("list\n");
      printf("prodcons\n");
      printf("prodcons_bb\n");
      printf("futest\n");
      printf("tscdf\n");
      return OK;
    } 

    if ((nargs == 2) && (strncmp(args[1], "hello", 5) != 0) 
      && (strncmp(args[1], "prodcons", 8) != 0) && (strncmp(args[1], "futest", 6) != 0)) {
      printf("hello\n"); 
      printf("list\n");
      printf("prodcons\n");
      printf("prodcons_bb\n");
      printf("futest\n");
      printf("tscdf\n");
      return OK;
    }

    /* This will go past "run" and pass the function/process name and its
    * arguments.
    */
    args++;
    nargs--;

    if (strncmp(args[0], "tscdf", 5) == 0) { // time stamped cdf
         resume( create(stream_proc, 1024, 20, "stream_proc", 2, nargs, args));
         return 0;
    }

    if(strncmp(args[0], "futest", 6) == 0) { 

      if (strncmp(args[1], "--free", 6) == 0) {
        future_free_test(nargs,args);
        return 0;
      }

      if (strncmp(args[1], "-f", 2) == 0) {
          if (nargs != 3) {
              printf("Syntax: run futest [-pc [g ...] [s VALUE ...]|-f NUMBER][--free]\n");
              return 0;           
          }
          int x =  atoi(args[2]);
          if (x == 0) {
              printf("Syntax: run futest [-pc [g ...] [s VALUE ...]|-f NUMBER][--free]\n");
              return 0;
          }

          return future_fib(nargs, args);
      }

      if (strncmp(args[1], "-pc", 3) == 0) {
         resume( create(future_prodcons, 1024, 20, "future_prodcons", 2, nargs, args));
         return 0;
      }

      printf("Syntax: run futest [-pc [g ...] [s VALUE ...]|-f NUMBER][--free]\n");
      return SYSERR;
    }


    if(strncmp(args[0], "hello", 5) == 0) {
    	if (nargs == 1 || nargs > 2) {
    		printf("Syntax: run hello [name]\n");
    		return 0;
    	}
      /* create a process with the function as an entry point. */
      resume (create((void *)xsh_hello, 4096, 20, "hello", 2, nargs, args));
      // printf("run hello %s\n", args[0]);
    }

    if(strncmp(args[0], "prodcons_bb", 10) == 0) {
      if (nargs != 5) {
        printf("Syntax: run prodcons_bb [# of producer processes] [# of consumer processes] [# of iterations the producer runs] [# of iterations the consumer runs]\n");
        return 0;
      }

      if (atoi(args[1]) * atoi(args[3]) != atoi(args[2]) * atoi(args[4])) {
        printf("Iteration Mismatch Error: the number of producer(s) iteration does not match the consumer(s) iteration\n");
        return 0;
      }
      resume( create(prodcons_bb, 1024, 20, "prodcons_bb", 2, nargs, args));
      return 0;
    } 
    else if (strncmp(args[0], "prodcons", 8) == 0) {
    	if (nargs == 2) {
    	for(int i = 0, n = strlen(args[1]); i < n; i++) {
			if(!isdigit(args[1][i])){ 
    		printf("Syntax: run prodcons [counter]\n");
    		return 0;
    	}}}
    	if (nargs > 2) {
    		printf("Syntax: run prodcons [counter]\n");
    		return 0;
    	}
      /* create a process with the function as an entry point. */
      resume (create((void *)xsh_prodcons, 4096, 20, "prodcons", 2, nargs, args));
      // printf("run prodcons %s\n", args[0]);
      return 0;
    }

    return 0;

}