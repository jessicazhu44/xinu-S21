#include <xinu.h>
#include <stdio.h>
#include <string.h>
#include <future.h>

future_t* future_alloc(future_mode_t mode, uint size, uint nelem)
{
  intmask mask;
  mask = disable();

  if (size == 0 || nelem == 0) {
  	restore(mask);
  	return (future_t*) SYSERR;
  }

  future_t *f = (future_t*) getmem(sizeof(future_t)); // this returns an address 'return (char *)(curr); ''

  // struct future_t *future1;
  f->data = (char *) getmem(size);
  f->state = FUTURE_EMPTY; 
  f->mode = mode;
  f->size = size;
  f->pid = -1;

  restore(mask);
  return f;
}

 // write your code here for future_free, future_get and future_set
	syscall future_free(future_t* f) {
		// free allocated 'future' use freemem
		// return syscall, either SYSERR or OK
		// kill() any process thats waiting for the future, use 'pid'
		if (f->pid > 0) {
			kill(f->pid);
		} 
		return freemem((char*)f, f->size);
	} 

	syscall future_get(future_t* f,  char* out) {
		// returns SYSERR if another process is already waiting on the target future
		//printf("f.pid result: %s\n", f->pid);
		if (f->pid > 0) {   
			return SYSERR;
		}
		// printf("hello\n");
		if (f->state == FUTURE_EMPTY) {
			// f->pid = 1;
			// printf("hello1\n");  // prints 'consumed24'???how to keep future_get waiting till
								// a set happens?
			f->state = FUTURE_WAITING;
			f->pid = getpid();
			suspend(f->pid);
			//out = f -> data;
			memcpy(out, f->data, f->size);
			return OK;
		}

		if (f->state == FUTURE_WAITING) { 
			// f->pid = 1;
			return SYSERR;
		}

		if(f->state == FUTURE_READY) {
			// when it is FUTURE_READY, the next future_get() call return set value 
			// and the future becomes EMPTY
			out = f->data; //return the set value
			
			f->state = FUTURE_EMPTY;
			f->pid = -1;
			return OK;
		}

		return OK;
	}

	syscall future_set(future_t* f, char* in) {
		// call future_set on empty 'future' returns to state FUTURE_READY
		if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_READY;
			f->data = in;
			return OK;
		} else if (f->state == FUTURE_READY) {
			// subsequent future_set call on FUTURE_READY should return ERROR
			return SYSERR;
		} else if (f -> state == FUTURE_WAITING) {
			f->data = in;
			f->state = FUTURE_EMPTY;
			resume(f->pid);
			f->pid = -1;
			return OK;
		}
		return OK;
	}



