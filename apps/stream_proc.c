#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include <stream.h>
#include <stdlib.h>
// #include "tscdf_input.h"
#include "tscdf.h"

uint pcport;

void stream_consumer(int32 id, future_t *f) {

  // Initialize tscdf time window
  int32* qarray;
  struct tscdf* tc = (struct tscdf*) tscdf_init(time_window);
  char *output = (char *)getmem(sizeof(char) * 64);
  int32 time_count = 0;
  int32 cts, cv;  
 
  de elem;  

  kprintf("stream_consumer id:%d (pid:%d)\n", id, getpid());
  while (1) {
    //   Get future values
    if (future_get(f, (char*) &elem) < 0){
      kprintf("line %d future get failed\n", __LINE__);
    }; 
    cts = elem.time; 
    cv = elem.value; 

  // Loop until exit condition reached
    if (cts == 0 && cv == 0) {
        break;
    }
    if (tscdf_update(tc, cts, cv) < 0) {
      printf("update error\n");
      return;
    }
    
    if (time_count < output_time - 1){
      time_count++; 
    } else{
        time_count = 0;
        qarray = tscdf_quartiles(tc);

        if(qarray == NULL) {
          kprintf("tscdf_quartiles returned NULL\n");
          continue;
        }

        sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
        kprintf("%s\n", output);
        freemem((char *)qarray, (6*sizeof(int32)));
        }
    }
  // Signal producer and exit
  kprintf("stream_consumer exiting\n");
  ptsend(pcport, getpid());
  return;
}


int stream_proc(int nargs, char* args[]) {

  char usage[] = "Usage: run tscdf_fq -s num_streams -w work_queue_depth -t time_window -o output_time\n";

  // Timing: Tick
  ulong secs, msecs, time;
  secs = clktime;
  msecs = clkticks;
  // Parse arguments

    char c;
    char* ch;
    int i;

    if (nargs != 9)
    {
        printf("%s", usage);
        return (-1);
    }
    else
    {
        i = nargs - 1;
        while (i > 0)
        {
            ch = args[i - 1];
            c = *(++ch);

            switch (c)
            {
            case 's':
                num_streams = atoi(args[i]);
                break;

            case 'w':
                work_queue_depth = atoi(args[i]);
                break;

            case 't':
                time_window = atoi(args[i]);
                break;

            case 'o':
                output_time = atoi(args[i]);
                break;

            default:
                printf("%s", usage);
                return (-1);
            }
            i -= 2;
        }
    }

  // Create port that allows `num_streams` outstanding messages
  if((pcport = ptcreate(num_streams)) == SYSERR) {
      printf("ptcreate failed\n");
      return(-1);
  }

  if((pcport = ptcreate(num_streams)) == SYSERR) {
      printf("ptcreate failed\n");
      return(-1);
  }
  // Create array to hold `num_streams` futures
  // pointer to an array that contains pointers to futures
  future_t **futures = (future_t**) getmem(sizeof(future_t*)*num_streams);

  /* Allocate futures and create consumer processes
   * Use `i` as the stream id.
   * Future mode        = FUTURE_QUEUE
   * Size of element    = sizeof(struct data_element)
   * Number of elements = work_queue_depth
   */
  for (i = 0; i < num_streams; i++) {
      futures[i] = future_alloc(FUTURE_QUEUE, sizeof(de), work_queue_depth);
      resume (create(stream_consumer, 4096, 20, "stream_consumer", 2, i, futures[i]));
  }

  // Parse input header file data and set future values
  int st, ts, v;
  char *a;
  de elem; 
  // const char *stream_input[n_input];
  // Parse input header file data and populate work queue
  
   for(i = 0; i < n_input; i++) {
    // "0\t10\t10"
    a = (char *)stream_input[i]; 
    st = atoi(a); // stream 
    while (*a++ != '\t');
    ts = atoi(a); // time stamp
    while (*a++ != '\t');
    v = atoi(a); //value

    elem.time = ts; 
    elem.value = v; 
    
    if (future_set(futures[st], (char *)&elem) <0 ){
      kprintf("line %d, future set failed", __LINE__);
    };

   }
  // Wait for all consumers to exit
    for(i=0; i < num_streams; i++) {
      uint32 pm;
      pm = ptrecv(pcport); // message 
      printf("process %d exited\n", pm);
    }
  // Free all futures
    int status = (int) freemem((char*)futures, sizeof(future_t)*num_streams);
    if (status < 1) {
      kprintf("free memory failed.\n");
    }
  // Timing: Tock+Report
    ptdelete(pcport, 0);

    time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
    printf("time in ms: %u\n", time);
  return 0;
}

/*
void stream_consumer(int32 id, stream_t *str) {

  int32* qarray;
  struct tscdf* tc = (struct tscdf*) tscdf_init(time_window);
  char *output = (char *)getmem(sizeof(char) * 64);
  int32 time_count = 0;

  kprintf("stream_consumer id:%d (pid:%d)\n", id, getpid());
  while (1) {
    wait(str->items);
    wait(str->mutex);
    // printf("stream id:%d, time: %d, value: %d\n", id, str.queue[tail].time, str.queue[tail].value);
    if (str->queue[str->tail].time == 0 && str->queue[str->tail].value == 0) {
        break;
    }
    if (tscdf_update(tc, str->queue[str->tail].time, str->queue[str->tail].value) < 0) {
      printf("update error\n");
      return;
    }
    // printf("stream_consumer id:%d (pid: %d), time: %d, value: %d\n",id, getpid(), str->queue[str->tail].time,str->queue[str->tail].value );
    str->tail =  (str->tail + 1) % work_queue_depth;
    signal(str->mutex);
    signal(str->spaces);


   if (time_count == output_time - 1) {

        time_count = 0;
        qarray = tscdf_quartiles(tc);

        if(qarray == NULL) {
          kprintf("tscdf_quartiles returned NULL\n");
          continue;
        }

        sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
        kprintf("%s\n", output);
        freemem((char *)qarray, (6*sizeof(int32)));
        // freemem((char *)output, (15*sizeof(char)));
        }
    else {
      time_count++;
    }
  }

  kprintf("stream_consumer exiting.\n");

  ptsend(pcport, getpid());
  // printf("final check point\n");

  return;
}

int32 stream_proc(int nargs, char* args[]) {
  
  ulong secs, msecs, time;
  secs = clktime;
  msecs = clkticks;

  // Parse arguments
    char usage[] = "Usage: run tscdf -s num_streams -w work_queue_depth -t time_window -o output_time\n";

    char c;
    char* ch;
    int i;

    if (nargs != 9)
    {
        printf("%s", usage);
        return (-1);
    }
    else
    {
        i = nargs - 1;
        while (i > 0)
        {
            ch = args[i - 1];
            c = *(++ch);

            switch (c)
            {
            case 's':
                num_streams = atoi(args[i]);
                break;

            case 'w':
                work_queue_depth = atoi(args[i]);
                break;

            case 't':
                time_window = atoi(args[i]);
                break;

            case 'o':
                output_time = atoi(args[i]);
                break;

            default:
                printf("%s", usage);
                return (-1);
            }
            i -= 2;
        }
    }
  if((pcport = ptcreate(num_streams)) == SYSERR) {
      printf("ptcreate failed\n");
      return(-1);
  }

  if((pcport = ptcreate(num_streams)) == SYSERR) {
      printf("ptcreate failed\n");
      return(-1);
  }

  // Create streams
  stream_t *s = (stream_t*) getmem(sizeof(stream_t)*num_streams);
  // create work queue of length work_queue_depth
  // struct data_element *de = (struct data_element*) getmem(sizeof(struct data_element)*work_queue_depth*num_streams);

  // Create consumer processes and initialize streams
  // Use `i` as the stream id.
  for (i = 0; i < num_streams; i++) { 
  	// initlization
  	s[i].spaces = semcreate(work_queue_depth);
  	s[i].items = semcreate(0);
  	s[i].mutex = semcreate(1);
  	s[i].head = 0;
  	s[i].tail = 0;
    s[i].queue = (struct data_element*) getmem(sizeof(struct data_element)*work_queue_depth);
  	// stream_consumer(int32 id, struct stream *str)
  	resume (create(stream_consumer, 4096, 20, "stream_consumer", 2, i, &s[i]));
 }

  int st, ts, v;
  char *a;
  // const char *stream_input[n_input];
  // Parse input header file data and populate work queue
   for(i = 0; i < n_input; i++) {
   	// "0\t10\t10"
   	a = (char *)stream_input[i];
    st = atoi(a);
    while (*a++ != '\t');
    ts = atoi(a);
    while (*a++ != '\t');
    v = atoi(a);

    wait(s[st].spaces);
    wait(s[st].mutex);
	// assign data element of stream_ID to queue[i]
    // int head = s[st].head;
    // printf("producer, time: %d, value %d\n", ts, v);
    s[st].queue[s[st].head].time = ts; 
    s[st].queue[s[st].head].value = v; 

	 s[st].head = (s[st].head + 1) % work_queue_depth;
	 signal(s[st].mutex);
    signal(s[st].items);
   }

  	for(i=0; i < num_streams; i++) {
      uint32 pm;
      pm = ptrecv(pcport); // message 
      printf("process %d exited\n", pm);
  	}

  	ptdelete(pcport, 0);

  	time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
  	printf("time in ms: %u\n", time);

  	return(0);
}
*/