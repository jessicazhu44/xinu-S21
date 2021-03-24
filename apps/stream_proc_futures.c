#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include <stream.h>
#include <stdlib.h>
#include <future_prodcons.h>
// #include "tscdf_input.h"
#include "tscdf.h"

// issues with port
uint pcport;
//int output_time;

void stream_consumer_future(int32 id, future_t *f) {

  // Initialize tscdf time window
  int32* qarray;
  struct tscdf* tc = (struct tscdf*) tscdf_init(time_window);
  char *output = (char *)getmem(sizeof(char) * 64);
  int32 time_count = 0;
  int32 cts, cv;  
  //de *d_e = (de*) getmem(sizeof(de)); 
  de elem;  
  // int c  = 0 ;
  kprintf("stream_consumer_future id:%d (pid:%d)\n", id, getpid());
  while (1) {
    //   Get future values
    //sleep(1);
    if (future_get(f, (char*) &elem) < 0){
      kprintf("line %d future get failed\n", __LINE__);
    }; 
    cts = elem.time; 
    cv = elem.value; 
    // kprintf("line33: id: %d,cts, %d, cv, %d\n", id,cts, cv);
  // Loop until exit condition reached
    if (cts == 0 && cv == 0) {
        break;
    }
    if (tscdf_update(tc, cts, cv) < 0) {
      printf("update error\n");
      return;
    }
    
    // kprintf("time count: %d, %d\n", time_count, output_time);
    if (time_count < output_time - 1){
      time_count++; 
      // kprintf("time count: %d, %d\n", time_count, output_time);
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
        // freemem((char *)output, (15*sizeof(char)));
        // Free tscdf time window
        //time_count = 0;
        }
        // c++;
        // if(c == 1000) break;
    }
  // Signal producer and exit
  kprintf("stream_consumer_future exiting\n");
  ptsend(pcport, getpid());
  return;
}


int stream_proc_futures(int nargs, char* args[]) {

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
//kprintf("hi %d\n", n_input);
  /* Allocate futures and create consumer processes
   * Use `i` as the stream id.
   * Future mode        = FUTURE_QUEUE
   * Size of element    = sizeof(struct data_element)
   * Number of elements = work_queue_depth
   */
  for (i = 0; i < num_streams; i++) {
      futures[i] = future_alloc(FUTURE_QUEUE, sizeof(de), work_queue_depth);
      /*
      futures[i].mode = FUTURE_QUEUE;
      futures[i].size = sizeof(struct data_element);
      futures[i].max_elems = work_queue_depth;
      */
      resume (create(stream_consumer_future, 4096, 20, "stream_consumer_future", 2, i, futures[i]));
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

    // kprintf("line 171: %d, %d, %d\n", st, ts, v);
    // wait(futures[st].spaces);
    // wait(s[st].mutex);

  // assign data element of stream_ID to queue[i]
    // int head = s[st].head;
    // printf("producer, time: %d, value %d\n", ts, v);

/*    de *d_e = (de*) getmem(sizeof(de));
    d_e->time = ts;
    d_e->value = v;*/
    elem.time = ts; 
    elem.value = v; 
    if (future_set(futures[st], (char *)&elem) <0 ){
      kprintf("line %d, future set failed", __LINE__);
      // return -1;
    };
    // futures[st].queue[s[st].head].time = ts; 
    // s[st].queue[s[st].head].value = v; 

   // s[st].head = (s[st].head + 1) % work_queue_depth;
   //  signal(s[st].mutex);
   //  signal(s[st].items);
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

