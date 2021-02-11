#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include <shprototypes.h>
#include <ctype.h>
#include <prodcons_bb.h>  
#include <stdlib.h>

int32 head;
int32 tail;
int32 arr_q[5];
// definition of array, semaphores and indices 
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
      return OK;
    } 

    if ((nargs == 2) && (strncmp(args[1], "hello", 5) != 0) && (strncmp(args[1], "prodcons", 8) != 0)) {
      printf("hello\n"); 
      printf("list\n");
      printf("prodcons\n");
      printf("prodcons_bb\n");
      return OK;
    }

    /* This will go past "run" and pass the function/process name and its
    * arguments.
    */
    args++;
    nargs--;

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