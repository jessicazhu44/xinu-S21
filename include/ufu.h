#ifndef _UFU_H_
#define _UFU_H_

#include <future.h>

#define UFU_PORT 7777
#define UFU_TIMEOUT 500
#define UFU_RETRIES 1

#define UFU_DEBUG 

#ifdef UFU_DEBUG 
#define errormsg(fmt, ...) printf("\033[31mERROR %20s:%-3d ()\033[39m " fmt, __FILE__, __LINE__, ##__VA_ARGS__);
#else
#define errormsg(fmt, ...)
#endif

/**
 * Types of UFU messages (UFU header)
 */
typedef enum {
  UFU_ALLOC, // !< future_alloc
  UFU_SET,   // !< future_set
  UFU_GET,   // !< future_get
  UFU_FREE,  // !< future_free
  UFU_ACK,   // !< Server's response of successful function call
  UFU_NACK,  // !< Server's response of failed function call
  UFU_EXIT   // !< Send exit signal to server
} ufumsg_type_t;

/**
 * Structure of ufumsg
 */
typedef struct ufumsg {
  ufumsg_type_t type;  // !< Type of UFU message
  future_t * address;  // !< Address of a future
  int value0;          // !< Primary value
  int value1;          // !< Secondary value
} ufumsg_t;


/**
 * Structure of ufu
 * (both client and server have one instance)
 */
typedef struct ufu {
  int32 slot;        // !< UDP connection slot
  uint16 locport;    // !< Local UDP port (client only)
  uint32 localip;    // !< Local IP
  uint16 remport;    // !< Remote UDP port (server only)
  uint32 remip;      // !< Remote IP
  char inbuf[1500];  // !< Buffer for incoming UDP messages
} ufu_t;


/**
 * ufu_init
 * Initialize a UDP connection to a remote machine for further operations
 *
 * @param     ip
 * @returns   syscall
 */
syscall ufu_init(char *ip);

/**
 * ufu_finalize
 * Finalize a UDP connection to previously connected remote machine
 *
 * @returns   syscall
 */
syscall ufu_finalize(void);

/**
 * ufu_sendexit
 * Send exit signal to server
 *
 * @returns   syscall
 */
syscall ufu_sendexit(void);

/**
 * ufu_alloc
 * Allocates a future on a remote machine
 * Synchronous remote procedure call (RPC)
 * Blocks until response arrives or timeout triggers
 *
 * @param     mode
 * @param     size
 * @param     nelems
 * @returns   future_t pointer or else SYSERR
 */
future_t* ufu_alloc(future_mode_t mode, uint size, uint nelems);

/**
 * ufu_free
 * Frees a previously allocated future on a remote machine
 * Synchronous remote procedure call (RPC)
 * Blocks until response arrives or timeout triggers
 *
 * @param     fut
 * @returns   syscall
 */
syscall ufu_free(future_t *fut);

/**
 * ufu_set
 * Sets a future on a remote machine with a value
 * Synchronous remote procedure call (RPC)
 * Blocks until response arrives or timeout triggers
 *
 * @param     fut
 * @param     in
 * @returns   syscall
 */
syscall ufu_set(future_t *fut, char *in);

/**
 * ufu_get
 * Gets the value of a future on a remote machine
 * Synchronous remote procedure call (RPC)
 * Blocks until response arrives or timeout triggers
 *
 * @param     fut
 * @param     out
 * @returns   syscall
 */
syscall ufu_get(future_t *fut, char *out);

/**
 * ufu_listen
 * Listen for incoming ufu messages and handle them appropriately
 * - Validate address for future access
 * - Forward the syscalls from the future API back to the client
 * - Always respond with either UFU_ACK or UFU_NACK
 * - Exit upon receiving UFU_EXIT
 */
void ufu_listen();

#endif /* _UFU_H */