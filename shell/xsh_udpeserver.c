/* xsh_udpeserver.c - xsh_udpeserver */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

#ifdef ETHER0
#define VERBOSE

/*------------------------------------------------------------------------
 * xsh_udpeserver - shell command that acts as a UDP echo server (is
 *			usually run in background)
 *------------------------------------------------------------------------
 */
shellcmd xsh_udpeserver(int nargs, char *args[])
{
	int32	retval;			/* return value from sys calls	*/
	uint32	localip;		/* local IP address		*/
	char	str[40];		/* temporary used for formatting */
	uint32	remip;			/* remote sender's IP address	*/
	uint16	remport;		/* remote sender's UDP port	*/
	char	buff[1500];		/* buffer for incoming reply	*/
	int32	msglen;			/* length of outgoing message	*/
	int32	slot;			/* slot in UDP table 		*/
	uint16	echoserverport= 7777;	/* port number for UDP echo	*/

	/* For argument '--help', emit a help message	*/

	if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
		printf("Use: %s\n\n", args[0]);
		printf("Description:\n");
		printf("\tBecome a UDP echo server\n");
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

	localip = getlocalip();
	if (localip == SYSERR) {
		fprintf(stderr,
			"%s: could not obtain a local IP address\n",
			args[0]);
		return 1;
	}

	/* register local UDP port */

	slot = udp_register(0, 0, echoserverport);
	if (slot == SYSERR) {
		fprintf(stderr, "%s: could not reserve UDP port %d\n",
				args[0], echoserverport);
		return 1;
	}
	sprintf(str, "%d.%d.%d.%d",
		(localip>>24)&0xff, (localip>>16)&0xff,
		(localip>>8)&0xff,        localip&0xff);
	printf("UDP Echo server running on %s:%d\n", str, echoserverport);

	/* Do forever: read an incoming datagram and send it back */

	while (TRUE) {
		retval = udp_recvaddr(slot, &remip, &remport, buff,
						sizeof(buff), 600000);

		if (retval == TIMEOUT) {
			continue;
		} else if (retval == SYSERR) {
			fprintf(stderr, "%s: error receiving UDP\n",
				args[0]);
			return 1;
		}
		msglen = retval;

#ifdef VERBOSE
		printf("> length=%d from=0 to=%d\n ", msglen, msglen - 1);
		int i;
		for (i = 0; i < msglen; i++) {
			printf(" %02x", buff[i]);
		}
		printf("     %s\n", buff);
#endif

		if ((msglen == 4 || msglen == 5) && strncmp(buff, "exit", 4) == 0) break;

		retval = udp_sendto(slot, remip, remport, buff, msglen);
		if (retval == SYSERR) {
			fprintf(stderr, "%s: udp_sendto failed\n",
				args[0]);
			return 1;
		}
	}
	return 0;
}
#endif