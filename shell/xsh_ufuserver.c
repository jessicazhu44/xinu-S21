/* xsh_ufuserver.c - xsh_ufuserver */

#include <xinu.h>
#include <stdio.h>
#include <string.h>
#include <ufu.h>

#ifdef ETHER0
#define VERBOSE

/*-----------------------------------------------------------------------------
 * xsh_ufuserver - shell command that acts as an UFU server
 *                 (is usually run in background)
 *-----------------------------------------------------------------------------
 */
shellcmd xsh_ufuserver(int nargs, char *args[]) {
  /* For argument '--help', emit a help message  */
  if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
    printf("Use: %s\n\n", args[0]);
    printf("Description:\n");
    printf("\tBecome an UFU server\n");
    printf("Options:\n");
    printf("\t--help\t display this help and exit\n");
    return 0;
  }

  /* Check for valid IP address argument */
  if (nargs != 1) {
    fprintf(stderr, "%s: no arguments expected\n", args[0]);
    fprintf(stderr, "Try '%s --help' for more information\n",
        args[0]);
    return 1;
  }

  ufu_listen();

  return 0;
}
#endif