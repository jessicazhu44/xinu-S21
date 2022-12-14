
#ifndef _FUTURE_H_
#define _FUTURE_H_

typedef enum {
  FUTURE_EMPTY,
  FUTURE_WAITING,
  FUTURE_READY
} future_state_t;

typedef enum {
  FUTURE_EXCLUSIVE,
  FUTURE_SHARED,
  FUTURE_QUEUE
} future_mode_t;

typedef struct future_t {
  char *data;
  uint size;
  future_state_t state;
  future_mode_t mode;
  pid32 pid;
  qid16 get_queue; 
  qid16 set_queue;
  uint16 max_elems; //size of the internal queue 
  uint16 count;
  uint16 head; // index of slot at which the next element will be removed
  uint16 tail;  //  index of next slot to which the next element will be added
} future_t;

/* Interface for the Futures system calls */
future_t* future_alloc(future_mode_t mode, uint size, uint nelems);
syscall future_free(future_t*);
syscall future_get(future_t*, char*);
syscall future_set(future_t*, char*);

#endif /* _FUTURE_H_ */