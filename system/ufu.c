 #include <xinu.h>
#include <ufu.h>

ufu_t ufu = {
  .slot = EMPTY,
  .locport = 52743
};

syscall ufu_init(char * ip) {
// Initialize an UFU connection to a remote server
  if (ufu.slot != EMPTY) {
    errormsg("Ufu was already initialized\n");
    return SYSERR;
  }

  if ((ufu.localip = getlocalip()) == SYSERR) {
    errormsg("Could not obtain a local IP address\n");
    return SYSERR;
  }

  if (dot2ip(ip, &ufu.remip) == SYSERR) {
    errormsg("Invalid IP address: %s\n", ip);
    return SYSERR;
  }

  if ((ufu.slot = udp_register(ufu.remip, UFU_PORT, ufu.locport)) == SYSERR) {
    errormsg("udp_register(%s, %d, %d) failed\n", ip, UFU_PORT, ufu.locport);
    return SYSERR;
  }

  return OK;
}

syscall ufu_finalize() {

  if (ufu.slot == EMPTY) {
    errormsg("Ufu is not initialized\n");
    return SYSERR;
  }

  if (udp_release(ufu.slot) == SYSERR) {
    errormsg("udp_release(%d) failed\n", ufu.slot);
    return SYSERR;
  }

  ufu.slot = EMPTY;

  return OK;
}


syscall ufu_sendexit() {
// Send exit signal to server
  int retval;
  ufumsg_t exit_msg;
  exit_msg.type = UFU_EXIT;

  if (udp_send(ufu.slot, (char *)&exit_msg, sizeof(exit_msg)) < 0){
    return SYSERR;
  } 
/*
  if (udp_release(ufu.slot) == SYSERR) {
    errormsg("udp_release(%d) failed\n", ufu.slot);
    return SYSERR;
  }
*/

    // int32 udp_recv(uid32 slot, char *buff, int32 len, uint32 timeout), returns msglen;
    retval = udp_recv(ufu.slot, ufu.inbuf, sizeof(ufu.inbuf), UFU_TIMEOUT); 
    
    if (retval == TIMEOUT || retval == SYSERR) {
      errormsg("error receiving UDP\n");
      return SYSERR;
    }

    ufumsg_t msg_rec;
    memcpy(&msg_rec, ufu.inbuf, sizeof(ufumsg_t));
    
    if(msg_rec.type == UFU_ACK) {
        return 0;
      } else {
        errormsg("not correct acknoledgement;\n");
        return SYSERR;
      }

  return 0;
}

future_t* ufu_alloc(future_mode_t mode, uint size, uint nelems) {
 // Frees a previously allocated future on a remote machine/servor
  // server sends back an acknoledgement to client 
 // Synchronous remote procedure call (RPC)
 // Blocks until response arrives or timeout triggers

  ufumsg_t alloc_send; // create an alloc_msg with value0 = size, value1 = nelems;
  alloc_send.type = UFU_ALLOC; 
  alloc_send.address = (future_t*) mode;
  alloc_send.value0 = size;
  alloc_send.value1 = nelems;

  if (udp_send(ufu.slot, (char *)&alloc_send, sizeof(alloc_send)) < 0){
    errormsg("sending message failed;\n");
    return (future_t*) SYSERR;
  } 

  int32 retval;
  ufumsg_t alloc_msg_rec;

    // int32 udp_recv(uid32 slot, char *buff, int32 len, uint32 timeout), returns msglen;
    retval = udp_recv(ufu.slot, ufu.inbuf, sizeof(ufu.inbuf), UFU_TIMEOUT); 

    if (retval == TIMEOUT || retval == SYSERR) {
      errormsg("error receiving UDP\n");
      return (future_t*) SYSERR;
    }

    // msglen = size(ufu_msg);
    memcpy(&alloc_msg_rec, ufu.inbuf, sizeof(ufumsg_t));
    
    if(alloc_msg_rec.type == UFU_ACK) {
      return alloc_msg_rec.address;
    } else {
      errormsg("not correct acknoledgement;\n");
      return (future_t*) SYSERR;
    }

  return (future_t*) SYSERR;
}

syscall ufu_free(future_t *fut) {
  /* free a remote future
ufu_free(address);            [FRE|ADDR]
                              \----------------> :
                                 [ACK|RET]       future_free(address);
                              <----------------/
*/
  ufumsg_t free_msg;
  free_msg.type = UFU_FREE;
  free_msg.address = fut;

  if (udp_send(ufu.slot, (char *)&free_msg, sizeof(free_msg)) < 0){
    errormsg("sending message failed;\n");
    return SYSERR;
  } 

  int32 retval;
  ufumsg_t msg_rec;

    // int32 udp_recv(uid32 slot, char *buff, int32 len, uint32 timeout), returns msglen;
    retval = udp_recv(ufu.slot, ufu.inbuf, sizeof(ufu.inbuf), UFU_TIMEOUT); 

    if (retval == TIMEOUT || retval == SYSERR) {
      errormsg("error receiving UDP\n");
      return SYSERR;
    }
    
    memcpy(&msg_rec, ufu.inbuf, sizeof(ufumsg_t));
     // recevies [ACK|RET]
    if(msg_rec.type == UFU_ACK) {
      return msg_rec.value0;
    } else {
      errormsg("not correct acknoledgement;\n");
      return SYSERR;
    }

  return SYSERR;
}

syscall ufu_set(future_t *fut, char *in) {
/*
ufu_set(address, in);         [SET|ADDR|IN]
                              \----------------> :
                                 [ACK|RET]       future_set(address, in);
                              <----------------/
*/
  ufumsg_t set_msg;
  set_msg.type = UFU_SET;
  set_msg.address = fut;
  set_msg.value0 = (int) in; // cast in into an integer

  if (udp_send(ufu.slot, (char *)&set_msg, sizeof(set_msg)) < 0){
    errormsg("sending message failed;\n");
    return SYSERR;
  } 

  int32 retval;
  ufumsg_t msg_rec;

    // int32 udp_recv(uid32 slot, char *buff, int32 len, uint32 timeout), returns msglen;
    retval = udp_recv(ufu.slot, ufu.inbuf, sizeof(ufu.inbuf), UFU_TIMEOUT); // !! CHECK SPELL 

    if (retval == TIMEOUT || retval == SYSERR) {
      errormsg("error receiving UDP\n");
      return SYSERR;
    }
    
    memcpy(&msg_rec, ufu.inbuf, sizeof(ufumsg_t));
     // recevies [ACK|RET]
    if(msg_rec.type == UFU_ACK) {
      return msg_rec.value0;
    } else {
      errormsg("not correct acknoledgement;\n");
      return SYSERR;
    }

  return SYSERR;
}

syscall ufu_get(future_t *fut, char *out) {
/*
ufu_get(address, *out);       [GET|ADDR|OUT]
                              \----------------> :
                                 [ACK|OUT]       future_get(address, *out);
                              <----------------/
*/
  ufumsg_t get_msg;
  get_msg.type = UFU_GET;
  get_msg.address = fut;
  get_msg.value0 = (int) out; // cast out into an integer

  if (udp_send(ufu.slot, (char *)&get_msg, sizeof(get_msg)) < 0){
    errormsg("sending message failed;\n");
    return SYSERR;
  } 

  int32 retval;
  ufumsg_t msg_rec;

    // int32 udp_recv(uid32 slot, char *buff, int32 len, uint32 timeout), returns msglen;
    retval = udp_recv(ufu.slot, ufu.inbuf, sizeof(ufu.inbuf), UFU_TIMEOUT); 

    if (retval == TIMEOUT || retval == SYSERR) {
      errormsg("error receiving UDP\n");
      return SYSERR;
    }
    
    memcpy(&msg_rec, ufu.inbuf, sizeof(ufumsg_t));
     // recevies [ACK|OUT]
    if(msg_rec.type == UFU_ACK) {
      memcpy(out, (char *) msg_rec.value1, sizeof(int));
      // out = (char *) msg_rec.value1;
      return msg_rec.value0; // 0 or syserr
    } else {
      errormsg("not correct acknoledgement;\n");
      return SYSERR;
    }

  return SYSERR;
}

void ufu_listen() {
/*
udp_send(slot, msg, msglen);
                                   \----------------------> udp_recvaddr(slot, &remip, &remport, buf, len, TIMEOUT);
                                                            :
                                                            udp_sendto(slot, remip, remport, buf, len);
udp_recv(slot, buf, len, TIMEOUT); <----------------------/
*/
char  str[40];
int32 retval; 
ufumsg_t msg_rec, msg_send;

  ufu.localip = getlocalip();
  if (ufu.localip == SYSERR) {
    printf("could not obtain a local IP address\n");
    return;
  }

  ufu.slot = udp_register(0,0, UFU_PORT);
  if (ufu.slot == SYSERR) {
    printf("could not reserve UDP port %d\n", UFU_PORT);
    return;
  }
  sprintf(str, "%d.%d.%d.%d",
    (ufu.localip>>24)&0xff, (ufu.localip>>16)&0xff,
    (ufu.localip>>8)&0xff,        ufu.localip&0xff);
  printf("UDP Echo server running on %s:%d\n", str, UFU_PORT);


    msg_send.type = UFU_ACK;

    while(TRUE) {

    // int32 udp_recvaddr(uid32 slot, uint32 *remip, uint16 *remport, char *buff, int32 len, uint32 timeout)
      retval = udp_recvaddr(ufu.slot, &ufu.remip, &ufu.remport, (char *)&ufu.inbuf, sizeof(ufu.inbuf), UFU_TIMEOUT);
      
    if (retval == TIMEOUT) {
      continue; 
        //errormsg("timeout\n");
        // return;      
    }

    if (retval == SYSERR) {
        errormsg("SYSERR\n");
        return;      
    }
      /*
      if (retval == TIMEOUT || retval == SYSERR) {
        errormsg("error receiving UDP\n");
        return;
      }
*/

      memcpy(&msg_rec, ufu.inbuf, sizeof(ufumsg_t));
      // printf("type: %d, %d, %d, %d\n", msg_rec.type, msg_rec.address, msg_rec.value0, msg_rec.value1);      
      if(msg_rec.type == UFU_ACK) { 
        errormsg("ACK not accepted message type\n");
        return;
      } 


      if(msg_rec.type == UFU_ALLOC) { 
/*ufu_alloc(ARGS);              [ALO|ARGS]
                              \----------------> :
                                 [ACK|ADDR]      future_alloc(...);
                              <----------------/
*/
        future_t* fut = future_alloc((future_mode_t) msg_rec.address, msg_rec.value0, msg_rec.value1); 
        msg_send.address = fut; 
        // printf("future_alloc(%d, %d, %d): 0x%08x\n", (future_mode_t) msg_rec.address, msg_rec.value0, msg_rec.value1, fut);
      }

      if(msg_rec.type == UFU_SET) { 
/*ufu_set(address, in);         [SET|ADDR|IN]
                              \----------------> :
                                 [ACK|RET]       future_set(address, in);
                              <----------------/
*/
        msg_send.value0 = future_set(msg_rec.address, (char *)msg_rec.value0);
        // printf("future_set(0x%08x, %d) succeeded\n", msg_rec.address, msg_rec.value0);
      }

      if(msg_rec.type == UFU_GET) { 
/*
ufu_get(address, *out);       [GET|ADDR|IN]
                              \----------------> :
                                 [ACK|OUT]       future_get(address, *out);
                              <----------------/
*/      // printf("line 340 : address: %d, value0 %d\n",msg_rec.address, msg_rec.value0);
        msg_send.value0 = future_get(msg_rec.address, (char *)msg_rec.value0);
        // printf("line 342 : value0 send %d\n",msg_send.value0);
        msg_send.value1 = (int) msg_rec.value0;

      }

      if(msg_rec.type == UFU_FREE) { 
/*
ufu_free(address);            [FRE|ADDR]
                              \----------------> :
                                 [ACK|RET]       future_free(address);
                              <----------------/
*/
        msg_send.value0 = future_free(msg_rec.address);
      }

      if(msg_rec.type == UFU_EXIT) { 
/*
ufu_sendexit();               [EXI]
                              \----------------> :
                                 [ACK]           // exit server loop
                              <----------------/
*/
        //The server should then exit the main loop and terminate gracefully.
        if (udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *)&msg_send, sizeof(msg_send)) < 0) {
            msg_send.type = UFU_NACK;
            udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *)&msg_send, sizeof(msg_send));
            errormsg("server sending message failed;\n");
            return;
        }
          
          break;
      }


      if (udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *)&msg_send, sizeof(msg_send)) < 0) {
          msg_send.type = UFU_NACK;
          udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *)&msg_send, sizeof(msg_send));
          errormsg("server sending message failed;\n");
          return;
      }

    }

    udp_release(ufu.slot);
    return;
  }



