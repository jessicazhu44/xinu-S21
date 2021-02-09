#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include <shprototypes.h>

shellcmd xsh_run(int nargs, char *args[]) {

    if ((nargs == 1) ) {
      printf("hello\n"); 
      printf("list\n");
      printf("prodcons\n");
      return OK;
    } 

    if ((nargs == 2) && (strncmp(args[1], "hello", 5) != 0) && (strncmp(args[1], "prodcons", 8) != 0)) {
      printf("hello\n"); 
      printf("list\n");
      printf("prodcons\n");
      return OK;
    }

    /* This will go past "run" and pass the function/process name and its
    * arguments.
    */
    args++;
    nargs--;

    if(strncmp(args[0], "hello", 5) == 0) {
    	if (nargs == 1) {
    		printf("run hello [name]\n");
    		return 0;
    	}
      /* create a process with the function as an entry point. */
      resume (create((void *)xsh_hello, 4096, 20, "hello", 2, nargs, args));
      // printf("run hello %s\n", args[0]);
    }
    if(strncmp(args[0], "prodcons", 8) == 0) {
    	if (nargs == 1) {
    		printf("run prodcons [counter]\n");
    		return 0;
    	}
      /* create a process with the function as an entry point. */
      resume (create((void *)xsh_prodcons, 4096, 20, "prodcons", 2, nargs, args));
      // printf("run prodcons %s\n", args[0]);
    }

    return 0;

}