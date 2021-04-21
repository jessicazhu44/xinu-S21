#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef FS
#include <fs.h>

static fsystem_t fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0 // Superblock
#define BM_BLK 1 // Bitmapblock

#define NUM_FD 16

filetable_t oft[NUM_FD]; // open file table
#define isbadfd(fd) (fd < 0 || fd >= NUM_FD || oft[fd].in.id == EMPTY)

#define INODES_PER_BLOCK (fsd.blocksz / sizeof(inode_t))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

/**
 * Helper functions
 */
int _fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEDIRECTBLOCKS) {
    errormsg("No indirect block support! (%d >= %d)\n", fileblock, INODEBLOCKS - 2);
    return SYSERR;
  }

  // Get the logical block address
  diskblock = oft[fd].in.blocks[fileblock];

  return diskblock;
}

/**
 * Filesystem functions
 */
int _fs_get_inode_by_num(int dev, int inode_number, inode_t *out) {
  int bl, inn;
  int inode_off;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %s)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl  = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(inode_t);

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(out, &block_cache[inode_off], sizeof(inode_t));

  return OK;

}

int _fs_put_inode_by_num(int dev, int inode_number, inode_t *in) {
  int bl, inn;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %d)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(inode_t))], in, sizeof(inode_t));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}

int fs_mkfs(int dev, int num_inodes) {
  int i;

  if (dev == dev0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  } else {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  } else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) { i++; }
  fsd.freemaskbytes = i / 8;

  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *) SYSERR) {
    errormsg("fs_mkfs memget failed\n");
    return SYSERR;
  }

  /* zero the free mask */
  for(i = 0; i < fsd.freemaskbytes; i++) {
    fsd.freemask[i] = '\0';
  }

  fsd.inodes_used = 0;

  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(fsystem_t));

  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  // Initialize all inode IDs to EMPTY
  inode_t tmp_in;
  for (i = 0; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    tmp_in.id = EMPTY;
    _fs_put_inode_by_num(dev0, i, &tmp_in);
  }
  fsd.root_dir.numentries = 0;
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
  }

  for (i = 0; i < NUM_FD; i++) {
    oft[i].state     = 0;
    oft[i].fileptr   = 0;
    oft[i].de        = NULL;
    oft[i].in.id     = EMPTY;
    oft[i].in.type   = 0;
    oft[i].in.nlink  = 0;
    oft[i].in.device = 0;
    oft[i].in.size   = 0;
    memset(oft[i].in.blocks, 0, INODEBLOCKS);
    oft[i].flag      = 0;
  }

  return OK;
}

int fs_freefs(int dev) {
  if (freemem(fsd.freemask, fsd.freemaskbytes) == SYSERR) {
    return SYSERR;
  }

  return OK;
}

/**
 * Debugging functions
 */
void fs_print_oft(void) {
  int i;

  printf ("\n\033[35moft[]\033[39m\n");
  printf ("%3s  %5s  %7s  %8s  %6s  %5s  %4s  %s\n", "Num", "state", "fileptr", "de", "de.num", "in.id", "flag", "de.name");
  for (i = 0; i < NUM_FD; i++) {
    if (oft[i].de != NULL) printf ("%3d  %5d  %7d  %8d  %6d  %5d  %4d  %s\n", i, oft[i].state, oft[i].fileptr, oft[i].de, oft[i].de->inode_num, oft[i].in.id, oft[i].flag, oft[i].de->name);
  }

  printf ("\n\033[35mfsd.root_dir.entry[] (numentries: %d)\033[39m\n", fsd.root_dir.numentries);
  printf ("%3s  %3s  %s\n", "ID", "id", "filename");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    if (fsd.root_dir.entry[i].inode_num != EMPTY) printf("%3d  %3d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
  printf("\n");
}

void fs_print_inode(int fd) {
  int i;

  printf("\n\033[35mInode FS=%d\033[39m\n", fd);
  printf("Name:    %s\n", oft[fd].de->name);
  printf("State:   %d\n", oft[fd].state);
  printf("Flag:    %d\n", oft[fd].flag);
  printf("Fileptr: %d\n", oft[fd].fileptr);
  printf("Type:    %d\n", oft[fd].in.type);
  printf("nlink:   %d\n", oft[fd].in.nlink);
  printf("device:  %d\n", oft[fd].in.device);
  printf("size:    %d\n", oft[fd].in.size);
  printf("blocks: ");
  for (i = 0; i < INODEBLOCKS; i++) {
    printf(" %d", oft[fd].in.blocks[i]);
  }
  printf("\n");
  return;
}

void fs_print_fsd(void) {
  int i;

  printf("\033[35mfsystem_t fsd\033[39m\n");
  printf("fsd.nblocks:       %d\n", fsd.nblocks);
  printf("fsd.blocksz:       %d\n", fsd.blocksz);
  printf("fsd.ninodes:       %d\n", fsd.ninodes);
  printf("fsd.inodes_used:   %d\n", fsd.inodes_used);
  printf("fsd.freemaskbytes  %d\n", fsd.freemaskbytes);
  printf("sizeof(inode_t):   %d\n", sizeof(inode_t));
  printf("INODES_PER_BLOCK:  %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS:  %d\n", NUM_INODE_BLOCKS);

  inode_t tmp_in;
  printf ("\n\033[35mBlocks\033[39m\n");
  printf ("%3s  %3s  %4s  %4s  %3s  %4s\n", "Num", "id", "type", "nlnk", "dev", "size");
  for (i = 0; i < NUM_FD; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) printf("%3d  %3d  %4d  %4d  %3d  %4d\n", i, tmp_in.id, tmp_in.type, tmp_in.nlink, tmp_in.device, tmp_in.size);
  }
  for (i = NUM_FD; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) {
      printf("%3d:", i);
      int j;
      for (j = 0; j < 64; j++) {
        printf(" %3d", *(((char *) &tmp_in) + j));
      }
      printf("\n");
    }
  }
  printf("\n");
}

void fs_print_dir(void) {
  int i;

  printf("%22s  %9s  %s\n", "DirectoryEntry", "inode_num", "name");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    printf("fsd.root_dir.entry[%2d]  %9d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
}

int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
}

int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/**
 * This is maybe a little overcomplicated since the lowest-numbered
 * block is indicated in the high-order bit.  Shift the byte by j
 * positions to make the match in bit7 (the 8th bit) and then shift
 * that value 7 times to the low-order bit to print.  Yes, it could be
 * the other way...
 */
void fs_printfreemask(void) { // print block bitmask
  int i, j;

  for (i = 0; i < fsd.freemaskbytes; i++) {
    for (j = 0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    printf(" ");
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}


/**
 * TODO: implement the functions below
 */
int fs_open(char *filename, int flags) {

  if (flags != O_RDONLY && flags != O_WRONLY && flags != O_RDWR) { 
    errormsg("wrong flags\n");
    return SYSERR;
  }

  int i = 0; 
  int file_inode;// file_node = fd
  while (i < DIRECTORY_SIZE) { // loop through root to find filename
      if(strcmp(fsd.root_dir.entry[i].name, filename) == 0) {
        // if filename match
        file_inode = fsd.root_dir.entry[i].inode_num;
        break;
      }
      i++;
    } 

  if(i == DIRECTORY_SIZE) {
    errormsg("file doesn't exist\n");
    return SYSERR;
  }

  // Return SYSERR if already open
  if (oft[file_inode].state == FSTATE_OPEN) {
    errormsg("file already open\n");
    return SYSERR;
  }

  // Using _get_inode_by_num function, 
  // (Only the inodes in the file table can be edited)
  // inode_t tmp_out;
  // int _fs_get_inode_by_num(int dev, int inode_number, inode_t *out)
  // if(_fs_get_inode_by_num(dev0, i, &tmp_out) <0) {
  //  errormsg("get inode failed\n");
  //  return SYSERR;
  // } 
  // make an entry of that inode in the file table. 
        // changes on OFT
    inode_t tmp_in;
    _fs_get_inode_by_num(dev0, file_inode, &tmp_in);
    oft[file_inode].state = FSTATE_OPEN;
    oft[file_inode].fileptr = 0;
    oft[file_inode].de = &fsd.root_dir.entry[i];
    oft[file_inode].in.id = tmp_in.id;
    oft[file_inode].in.type = tmp_in.type;
    oft[file_inode].in.nlink = tmp_in.nlink;
    oft[file_inode].in.device = tmp_in.device;
    oft[file_inode].in.size = tmp_in.size;
    memset(oft[file_inode].in.blocks, *tmp_in.blocks, INODEBLOCKS);
    oft[file_inode].flag = flags;


  // oft[file_inode].state = FSTATE_OPEN;
  // oft[file_inode].de = &fsd.root_dir.entry[i];
  // oft[file_inode].in.id = file_inode;
  // oft[file_inode].in.type = INODE_TYPE_FILE;
  // oft[file_inode].in.nlink = 1;
  // oft[file_inode].flag = flags;
  // oft[i].de = &fsd.root_dir.entry[i];
  // oft[i].in = tmp_out;

  //Return file descriptor on success
  return file_inode;
}

int fs_close(int fd) {
  // Return SYSERR if already closed
  if (oft[fd].state == FSTATE_CLOSED) {
    errormsg("file already closed\n");
    return SYSERR;
  }
  // Change the state of an open file to FSTATE_CLOSED in the file table
  oft[fd].state = FSTATE_CLOSED;
  // Return OK on success
  return OK;
}

int fs_create(char *filename, int mode) {
  // dont support directory
  // if (mode != INODE_TYPE_FILE) {
  if (mode != O_CREAT) {
    errormsg("Directory creation not supported\n");
    return SYSERR;
  }
  // When a new file or directory is created, the root directory is updated accordingly.
  // Return SYSERR if root directory is already full
  if (fsd.root_dir.numentries == DIRECTORY_SIZE) {
    errormsg("Directory full\n");
    return SYSERR;
  }

  // Return SYSERR if the filename already exists
  for (int i = 0; i < DIRECTORY_SIZE; i++) {
    if (strcmp(fsd.root_dir.entry[i].name, filename) == 0) {
      errormsg("Filename %s already exists\n", filename);
      return SYSERR;
    }
  }

  // to create and add a file
  int i = 0; //entry.inode_num = oft[i]
  while(i < DIRECTORY_SIZE) {  
    // Determine next available inode number
    if (fsd.root_dir.entry[i].inode_num == EMPTY) {
        // Add inode to the file system and Open file
        fsd.root_dir.entry[i].inode_num = i; 
        // void *memcpy(void *dest, const void * src, size_t n)
        memcpy(fsd.root_dir.entry[i].name, filename,FILENAMELEN); //strcpy acts on value \0 or NULL
        fsd.root_dir.numentries++;
        fsd.inodes_used++;
        // fs_print_inode(i);
  
        // update inode info - changes directly on data blocks
        inode_t tmp_in;
        _fs_get_inode_by_num(dev0, i, &tmp_in);
        tmp_in.id = i;
        tmp_in.type = INODE_TYPE_FILE;
        tmp_in.nlink = 1;
        tmp_in.device = 0; 
        tmp_in.size = 0; 
        _fs_put_inode_by_num(dev0, i, &tmp_in);

        fs_open(filename, O_RDWR);
        // Return file descriptor on success
        return i;
    }
    i++;
  }
  return i;
}

int fs_seek(int fd, int offset) {
    // each file has 10 data blocks, each data block has 512 bytes;
  // an offset shouldn't go out of file's total bytes (5120 bytes)
  // Return SYSERR if the offset would go out of bounds
  
  if(oft[fd].state != FSTATE_OPEN) {
    errormsg("file is not open\n");
    return SYSERR;        
  }

  if(offset < 0) {
    errormsg("negative offset not allowed\n");
    return SYSERR;    
  }

  if (offset >= oft[fd].in.size) { 
    errormsg("offset out of bound\n");
    return SYSERR;
  }

  // set offset
  oft[fd].fileptr = offset;
  // Return OK on success
  return OK;
}

int fs_read(int fd, void *buf, int nbytes) {
  // if file is not open, return SYSERR
  if (oft[fd].state != FSTATE_OPEN) {
    errormsg("file is not open\n");
    return SYSERR;
  } 

  if (oft[fd].flag == 1) { // write only
    errormsg("permission denied\n");
    return SYSERR;
  }

  // Read file contents stored in the data blocks, 
  // always read starting from fileptr

  int bl  = oft[fd].fileptr / MDEV_BLOCK_SIZE; // <- divide by 512;calculate from which block to start
  int inn = oft[fd].fileptr % MDEV_BLOCK_SIZE;  // find offset

  // int bs_bread(int dev, int block, int offset, void *buf, int len): OK
  // int _fs_fileblock_to_diskblock(int dev, int fd, int fileblock)    disk_block_number

  // check if read bytes doesnt go out of file size
  if ((oft[fd].in.size - oft[fd].fileptr) < nbytes) {
      nbytes = oft[fd].in.size - oft[fd].fileptr;
  }

  if ((MDEV_BLOCK_SIZE - inn) >= nbytes) {
    // bytes fits into the space left in first block
    //kprintf("line 502, hello\n");
    bs_bread(dev0, _fs_fileblock_to_diskblock(0, fd, bl),inn, buf, nbytes);
    oft[fd].fileptr += nbytes; 
    return nbytes;
  }

  // else, bytes cant fit all into space left in the first block
    // 1)read in partially from first block
  bs_bread(dev0, _fs_fileblock_to_diskblock(0, fd, bl),inn, buf, (MDEV_BLOCK_SIZE - inn));
  bl++;
    // 2) read block by block till the end
  int bytes_to_read = nbytes - (MDEV_BLOCK_SIZE - inn);

//kprintf("line 513: bytes_to_read: %d, remainder: %d\n", bytes_to_read, (MDEV_BLOCK_SIZE - inn));

  while (bytes_to_read > MDEV_BLOCK_SIZE) {
    bs_bread(dev0, _fs_fileblock_to_diskblock(0, fd, bl),0, buf+nbytes-bytes_to_read, MDEV_BLOCK_SIZE);
    bl++;
    bytes_to_read -= MDEV_BLOCK_SIZE;
  }

    // read the final part that's less than MDEV_BLOCK_SIZE
    // bl++;
  // kprintf("line 584: bytes_to_read: %d, left to read: %d,  bl: %d\n", bytes_to_read, nbytes - bytes_to_read,bl);
    bs_bread(dev0, _fs_fileblock_to_diskblock(0, fd, bl),0, buf+nbytes-bytes_to_read, bytes_to_read);
    oft[fd].fileptr += nbytes; 
  // Return the bytes read or SYSERR
  return nbytes;
}

int fs_write(int fd, void *buf, int nbytes) {
  // Write content to the data blocks of the file
  // Allocate new blocks if needed (do not exceed the maximum limit)
  // Do not forget to update size and fileptr
  // Return the bytes written or SYSERR

  if (oft[fd].state != FSTATE_OPEN) {
    errormsg("file is not open\n");
    return SYSERR;
  } 

  if (oft[fd].flag == 0) { // read only
    errormsg("permission denied\n");
    return SYSERR;
  }

  //kprintf("line 547: %d, %d\n", (MDEV_BLOCK_SIZE*INODEDIRECTBLOCKS) - oft[fd].fileptr, nbytes);
  // calculate the amount of space left to store more data
  // ensure it doesnt go out of bound
  if((MDEV_BLOCK_SIZE*INODEDIRECTBLOCKS) - oft[fd].fileptr < nbytes) {
    nbytes = (MDEV_BLOCK_SIZE*INODEDIRECTBLOCKS)- oft[fd].fileptr;
  }

  int bl = oft[fd].fileptr / MDEV_BLOCK_SIZE; // <- divide by 512; calculate block index
  int inn = oft[fd].fileptr % MDEV_BLOCK_SIZE; // find which byte in a data block

  // if data can fit into the first block
  if ((MDEV_BLOCK_SIZE - inn) >= nbytes) {
    // int bs_bwrite(int bsdev, int block, int offset, void *buf, int len);
    bs_bwrite(dev0, _fs_fileblock_to_diskblock(0, fd, bl), inn, buf, nbytes);
    fs_setmaskbit(oft[fd].in.blocks[bl]);
    oft[fd].in.size += nbytes;
    oft[fd].fileptr += nbytes;    
    // kprintf("line 562: hi\n");
    return nbytes;
  }

  // if data can't fit into the first block
  // fill in the first block first
  bs_bwrite(dev0, _fs_fileblock_to_diskblock(0, fd, bl), inn, buf, (MDEV_BLOCK_SIZE - inn));
  fs_setmaskbit(oft[fd].in.blocks[bl]);
  // fill other bytes into the following blocks
  int bytes_to_write = nbytes - (MDEV_BLOCK_SIZE - inn);
  // kprintf("line 567: bytes_to_write: %d, remainder: %d\n", bytes_to_write, (MDEV_BLOCK_SIZE - inn));
  bl++;

  while (bytes_to_write > MDEV_BLOCK_SIZE) {
    bs_bwrite(dev0, _fs_fileblock_to_diskblock(0, fd, bl), 0, buf+(nbytes - bytes_to_write), MDEV_BLOCK_SIZE);
    fs_setmaskbit(oft[fd].in.blocks[bl]);
    bl++;
    bytes_to_write -= MDEV_BLOCK_SIZE;
  }

  // read the final part that's less than MDEV_BLOCK_SIZE
  // bl++;
   // kprintf("line 584: bytes_to_write: %d, left to write: %d,  bl: %d\n", bytes_to_write, nbytes - bytes_to_write,bl);
  bs_bwrite(dev0, _fs_fileblock_to_diskblock(0, fd, bl), 0, buf+(nbytes - bytes_to_write), bytes_to_write);

  oft[fd].in.size += nbytes;
  oft[fd].fileptr += nbytes;
 //kprintf("line 591: nbytes: %d, oft[fd].in.size: %d,  oft[fd].fileptr: %d\n", nbytes, oft[fd].in.size,oft[fd].fileptr);

  return nbytes;
}

int fs_link(char *src_filename, char* dst_filename) {
  // bool src_exist = 0;
  int src_inode = -1;

  for (int i = 0; i < DIRECTORY_SIZE; i++) { 
    if (strcmp(fsd.root_dir.entry[i].name, dst_filename) == 0 ) {
      errormsg("No duplicate link names are allowed\n");
      return SYSERR;         
    }

    if (strcmp(fsd.root_dir.entry[i].name, src_filename) == 0) {
      src_inode = fsd.root_dir.entry[i].inode_num;
    }
  }

  if(src_inode < 0) {
      errormsg("Source file doesn't exist\n");
      return SYSERR;      
  }
     
/*
  // Search the src_filename in the root directory
  for (int i = 0; i < DIRECTORY_SIZE; i++) {
    if (strcmp(fsd.root_dir.entry[i].name, dst_filename) == 0) {
      // Return SYSERR if the filename already exists
      printf("No duplicate link names are allowed\n");
      return SYSERR;
    }
  }
  
  
  for (int i = 0; i < DIRECTORY_SIZE; i++) {
    if (strcmp(fsd.root_dir.entry[i].name, src_filename) == 0) {
        src_inode = i;
        break;
    }
  }
*/


  for (int i = 0; i < DIRECTORY_SIZE; i++) {
    if (fsd.root_dir.entry[i].inode_num == EMPTY) {
      // Copy the inode num of src_filename to 
      // the 1st new entry with dst_filename as its filename
      fsd.root_dir.entry[i].inode_num = src_inode;
      memcpy(fsd.root_dir.entry[i].name, dst_filename,FILENAMELEN);

/*
      // update inode in oft
      oft[src_inode].in.nlink++;
*/      
      inode_t tmp_in;
      _fs_get_inode_by_num(dev0, src_inode, &tmp_in);
      tmp_in.nlink++;
      _fs_put_inode_by_num(dev0, src_inode, &tmp_in);
      
      fsd.root_dir.numentries++;
        //fs_print_dir();
      return OK;
    }
  }

    errormsg("no space for links\n");
    return SYSERR;

}


// ?? Both root directory and OFT has inode, when we remove a file using unlink,
// do we need to update the inodes in both OFT and ROOT?


int fs_unlink(char *filename) {
  // Search filename in the root directory
  int file_inode;
  int i = 0;
  while (i < DIRECTORY_SIZE) {
    if (strcmp(fsd.root_dir.entry[i].name, filename) == 0) {
        file_inode = fsd.root_dir.entry[i].inode_num;
        fsd.root_dir.entry[i].inode_num = EMPTY;
        memset(fsd.root_dir.entry[i].name, 0,FILENAMELEN);
        fsd.root_dir.numentries--;
        break;
    }
    i++;    
  }

  if (i == DIRECTORY_SIZE) {
    errormsg("no such file\n");
      // fs_print_dir();
    return SYSERR;
  }

  inode_t tmp_out;
  _fs_get_inode_by_num(dev0, file_inode, &tmp_out); 
  tmp_out.nlink--;

  if(tmp_out.nlink == 0) {
    // remove the entry from oft 
    // empty the data blocks of the file using fs_clearmaskbit
    // If the nlinks of the inode is just 1, 
    // then delete the respective inode along with its data blocks as well
    oft[file_inode].state = 0;
    oft[i].fileptr   = 0;
    oft[i].in.type   = 0;
    oft[i].in.nlink  = 0;
    oft[i].in.device = 0;
    oft[i].in.size   = 0;
    memset(oft[i].in.blocks, 0, INODEBLOCKS);
    oft[i].flag      = 0;

    for(int i = 0; i < INODEDIRECTBLOCKS; i++) {
      fs_clearmaskbit(tmp_out.blocks[i]);
    }

    fsd.inodes_used--;
  }

  _fs_put_inode_by_num(dev0, file_inode, &tmp_out);


/*
  if(tmp_out.nlink == 0) {
      // If the nlinks of the respective inode left is 0, 
  // just remove the entry in the root directory
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0,FILENAMELEN);

    oft[file_inode]
  }

    if(tmp_out.nlink > 0) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0,FILENAMELEN);
    fsd.root_dir.numentries--;

    tmp_out.id = EMPTY;
    tmp_out.nlink = 0;
    memset(tmp_out.blocks, 0, INODEBLOCKS);
  }


  // If the nlinks of the respective inode is more than 1, 
  // just remove the entry in the root directory
  if(tmp_out.nlink > 1) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0,FILENAMELEN);
    
    
  }

  // If the nlinks of the inode is just 1, 
  // then delete the respective inode along with its data blocks as well
  if(tmp_out.nlink == 1) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0,FILENAMELEN);
    fsd.root_dir.numentries--;

    tmp_out.id = EMPTY;
    tmp_out.nlink = 0;
    memset(tmp_out.blocks, 0, INODEBLOCKS);
  }
  
  inode_t tmp_out;
  // int _fs_get_inode_by_num(int dev, int inode_number, inode_t *out)
  if(_fs_get_inode_by_num(dev0, file_inode, &tmp_out) <0) {
    errormsg("get inode failed\n");
    return SYSERR;
  } 


  // printf("line 604: %s, %d\n",tmp_out.type, tmp_out.nlink );

  // If the nlinks of the respective inode is more than 1, 
  // just remove the entry in the root directory
  if (tmp_out.nlink > 1) {
    //  _fs_get_inode_by_num(dev0, inode_num, &tmp_in);
    // kprintf("line 613: %d\n", tmp_out.nlink);
    tmp_out.nlink--; 

    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
    _fs_put_inode_by_num(dev0, file_inode, &tmp_out);
    
    //??? after removal, do we need to update inodes in both oft and root_directory?
    //update inode info in OFT
    oft[fd].in.nlink++;
    // remove the entry in OFT



    // fs_print_inode(file_inode);

  }

  // If the nlinks of the inode is just 1, 
  // then delete the respective inode along with its data blocks as well
  if (tmp_out.nlink == 1) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
    // _fs_put_inode_by_num(int dev, int inode_number, inode_t *in)
    inode_t tmp_in; //?? Do i need to specifically set all inode parameters to NULL??
    _fs_put_inode_by_num(dev0, file_inode, &tmp_in);
    fsd.root_dir.numentries--;
  }  
*/
  //fs_print_dir();
  return OK;
}

#endif /* FS */