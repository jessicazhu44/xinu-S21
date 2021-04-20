/* xsh_ufuclient.c - xsh_ufuclient */

#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <future.h>
#include <ufu.h>

#ifdef ETHER0

/*-----------------------------------------------------------------------------
 * xsh_ufuclient - shell command that can interact with remote futures over UDP
 *-----------------------------------------------------------------------------
 */

void usage(char *arg0) {
  printf("Usage: %s REMOTEIP NOF_FUTURES COMMAND...\n\n", arg0);
  printf("COMMAND\n");
  printf("  %-10s  %s\n", "i",        "Initialize UFU connection");
  printf("  %-10s  %s\n", "a ID",     "Allocate future with specified ID");
  printf("  %-10s  %s\n", "s ID NUM", "Set value NUM to future with specified ID");
  printf("  %-10s  %s\n", "g ID",     "Get value from future with specified ID");
  printf("  %-10s  %s\n", "f ID",     "Free future with specified ID");
  printf("  %-10s  %s\n", "e",        "Send exit signal to server");
  printf("  %-10s  %s\n", "q",        "Shutdown UFU connection");
}

shellcmd xsh_ufuclient(int nargs, char *args[]) {

  int i;
  int tmp;
  int nof_ufus;
  future_t **futures;

  if (nargs < 3) {
    usage(args[0]);
    return 0;
  }

  nof_ufus = atoi(args[2]);
  futures = (future_t **) getmem(sizeof(future_t *) * nof_ufus);
  for (i = 0; i < nof_ufus; i++) futures[i] = NULL;

  printf("Running with %d UFUs\n", nof_ufus);

  i = 3;
  while (i < nargs) {
    if (strcmp(args[i], "i") == 0) {
      // Format: i
      if (ufu_init(args[1]) == SYSERR) {
        printf("ufu_init(%s) failed\n", args[1]);
      } else {
        printf("ufu_init(%s)\n", args[1]);
      }
    } else if (strcmp(args[i], "e") == 0) {
      if (ufu_sendexit() == SYSERR) {
        printf("ufu_sendexit() failed\n");
      } else {
        printf("ufu_sendexit()\n");
      }
    } else if (strcmp(args[i], "q") == 0) {
      if (ufu_finalize() == SYSERR) {
        printf("ufu_finalize() failed\n");
      } else {
        printf("ufu_finalize()\n");
      }
    } else if (strcmp(args[i], "a") == 0) {
      // Format: a ID
      i++;
      if (atoi(args[i]) >= nof_ufus || futures[atoi(args[i])] != NULL || (futures[atoi(args[i])] = ufu_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1)) == NULL) {
        printf("alloc futures[%d] failed\n", atoi(args[i]));
      } else {
        printf("alloc futures[%d]: 0x%08x\n", atoi(args[i]), futures[atoi(args[i])]);
      }
    } else if (strcmp(args[i], "s") == 0) {
      // Format: s ID NUM
      i++;
      tmp = atoi(args[i + 1]);
      if (ufu_set(futures[atoi(args[i])], (char *) &tmp) == SYSERR) {
        printf("set futures[%d] failed\n", atoi(args[i]));
      } else {
        printf("set futures[%d] <- %d\n", atoi(args[i]), tmp);
      }
      i++;
    } else if (strcmp(args[i], "g") == 0) {
      // Format: g ID
      i++;
      if (ufu_get(futures[atoi(args[i])], (char *) &tmp) == SYSERR) {
        printf("get futures[%d] failed\n", atoi(args[i]));
      } else {
        printf("get futures[%d] -> %d\n", atoi(args[i]), tmp);
      }
    } else if (strcmp(args[i], "f") == 0) {
      // FormaT: f ID
      i++;
      if (ufu_free(futures[atoi(args[i])]) == SYSERR) {
        printf("free futures[%d] failed\n", atoi(args[i]));
      } else {
        printf("free futures[%d]\n", atoi(args[i]));
      }
    } else {
      printf("Illegal argument args[%d]: %c\n", i, args[i]);
    }
    i++;
  }

  // Cleanup
  freemem((char *) futures, sizeof(future_t *) * nof_ufus);

  return 0;
}
#endif