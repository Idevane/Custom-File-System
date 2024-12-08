#include "fs.h"
#include <string.h>
#include <libgen.h>

struct super_block super_block;
struct inode *inodes;
struct disk_block *disk_blocks;
struct myopenfile openfiles[MAX_FILES];
unsigned char* fs;

void mapfs(int fd){
  if ((fs = mmap(NULL, FSSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == NULL){
      perror("mmap failed");
      exit(EXIT_FAILURE);
  }
}


void unmapfs(){
  munmap(fs, FSSIZE);
}


void formatfs(){
   printf("Formatting file\n");
   int size_without_superblock = FSSIZE - sizeof(struct super_block);
   super_block.inodes = (size_without_superblock/10)/(sizeof(struct inode));
   super_block.blocks = (size_without_superblock-size_without_superblock/10)/(sizeof(struct disk_block));

   inodes = malloc(super_block.inodes*sizeof(struct inode));
   disk_blocks = malloc(super_block.blocks*sizeof(struct disk_block));

   for (size_t i = 0; i < super_block.inodes; i++)
    {
        strcpy(inodes[i].name, "");
        inodes[i].next = -1;
        inodes[i].dir = 0;
    }

    for (size_t i = 0; i < super_block.blocks; i++)
    {
        strcpy(disk_blocks[i].data, "");
        disk_blocks[i].next = -1;
    }

    createroot();
}

void mysync(const char* target) {
    /**
     * @brief Will load the UFS that is currently on the memory into a file named target.
     *  The file could be loaded in the future using resync.
     */
    FILE *file;
    file = fopen(target, "w+");
    fwrite(&super_block, sizeof(struct super_block), 1, file);
    fwrite(inodes, super_block.inodes*sizeof(struct inode), 1, file);
    fwrite(disk_blocks, super_block.blocks*sizeof(struct disk_block), 1, file);
    fclose(file);
//    myfsys = target;
}

void createroot() {
    
    int zerofd = allocate_file(sizeof(struct mydirent), "root");
    if (zerofd != 0 ) {
        errno = 131;
	printf("Error formatting file. Error: %d\n", errno);
        return;// -1;
    }
    inodes[zerofd].dir = 1;
    struct mydirent* rootdir = malloc(sizeof(struct mydirent));
    for (size_t i = 0; i < MAX_DIR_SIZE; i++)
    {
        rootdir->fds[i] = -1;
    }
    strcpy(rootdir->d_name, "root");
    rootdir->size = 0;
    char* rootdiraschar = (char*)rootdir;
    for (size_t i = 0; i < sizeof(struct mydirent); i++)
    {
        writebyte(zerofd, i, rootdiraschar[i]);
    }
    free(rootdir);
}

int myopen(const char *pathname, int flags) {
    /**
     * @brief Open a file at the given path.
     * The opened file will be added into 'openfiles' struct array and this instance will be used to get the pointer of the file.
     */
    char str[80];
    strcpy(str, pathname);
    char *token;
    const char s[2] = "/";
    token = strtok(str, s);
    char currpath[NAME_SIZE] = "";
    char lastpath[NAME_SIZE] = "";
    while(token != NULL ) {
        if (token != NULL) {
            strcpy(lastpath, currpath);
            strcpy(currpath, token);
        }
        token = strtok(NULL, s);
    }
    for (size_t i = 0; i < super_block.inodes; i++)
    {
        if (!strcmp(inodes[i].name, currpath)) {
            if (inodes[i].dir!=0) {
                errno = 21;
                return -1;
            }
            openfiles[i].fd = i;
            openfiles[i].pos = 0;
            return i;
        }
    }
    if (flags != O_CREAT) {
        errno = 2;
        return -1;
    }
    int i = mycreatefile(lastpath, currpath);
    openfiles[i].fd = i;
    openfiles[i].pos = 0;
    return i;
}

int myclose(int myfd) {
    openfiles[myfd].fd = -1;
    openfiles[myfd].pos = -1;
}

void writebyte(int fd, int opos, char data) {
    /**
     * @brief Write a SINGLE byte into a disk block.
     * The function calculates the correct relevant block (rb) that is needed to be accessed.
     * if the position that is needed to be wrriten is out of the bounds of the file,
     * allocate a new disk block for it.
     */
    int pos = opos;
    int rb = inodes[fd].next;
    while (pos>=BLOCK_SIZE) {
        pos-=BLOCK_SIZE;
        if (disk_blocks[rb].next==-1) {
            errno = 131;
	    printf("Error writing to disk. Error %d\n", errno);
            return;// -1;
        } else if (disk_blocks[rb].next == -2) { // the current block is the last block, so we allocate a new block
            disk_blocks[rb].next = find_empty_block();
            rb = disk_blocks[rb].next;
            disk_blocks[rb].next = -2;
        } else {
            rb = disk_blocks[rb].next;
        }
    }
    if (opos>inodes[fd].size) {
        inodes[fd].size = opos+1;
    }
    disk_blocks[rb].data[pos] = data;
}

int allocate_file(int size, const char* name) {
    /**
     * @brief This function will allocate new inode and enough blocks for a new file.
     * (One inode is allocated, the amount of needed blocks is calculated)
     *
     */
    if (strlen(name)>7) {
        errno = 36;
        return -1;
    }
    int inode = find_empty_inode();
    if (inode == -1) {
        errno = 28;
        return -1;
    }
    int curr_block = find_empty_block();
    if (curr_block == -1) {
        errno = 28;
        return -1;
    }
    inodes[inode].size = size;
    inodes[inode].next = curr_block;
    disk_blocks[curr_block].next = -2;
    strcpy(inodes[inode].name, name);
    if (size>BLOCK_SIZE) {  // REQUIRES TESTS
        int allocated_size = -(3*BLOCK_SIZE)/4;
        //int bn = size/BLOCK_SIZE;
        int next_block;
        while (allocated_size<size)
        {
            next_block = find_empty_block();
            if (next_block == -1) {
                errno = 28;
                return -1;
            }
            disk_blocks[curr_block].next = next_block;
            curr_block = next_block;
            allocated_size+=BLOCK_SIZE;
        }
    }
    disk_blocks[curr_block].next = -2;

    return inode;
}

int find_empty_block() {
    for (size_t i = 0; i < super_block.blocks; i++)
    {
        if (disk_blocks[i].next == -1) {
            return i;
        }
    }
    return -1;
}

int find_empty_inode() {
    for (size_t i = 0; i < super_block.inodes; i++)
    {
        if (inodes[i].next == -1) {
            return i;
        }

    }

    return -1;
}

void loadfs(const char* target){
 FILE *file;
 file = fopen(target, "r");
 if (!file) {
      printf("Error loading file system.\n");
      return;// -1;
 }
 fread(&super_block, sizeof(super_block), 1, file);
 inodes = malloc(super_block.inodes*sizeof(struct inode));
 disk_blocks = malloc(super_block.blocks*sizeof(struct disk_block));
 fread(inodes, super_block.inodes*sizeof(struct inode), 1, file);
 fread(disk_blocks, super_block.blocks*sizeof(struct disk_block), 1, file);
 fclose(file);
 printf("File system successfully loaded.\n");
}

void printfs_dsk(char* target) {
    /**
     * @brief Function used for debugging, print information about the UFS from a file on the disk.
     */
    FILE *file;
    file = fopen(target, "r");
    struct super_block temp_super_block;
    fread(&temp_super_block, sizeof(super_block), 1, file);
    struct inode *temp_inodes = malloc(temp_super_block.inodes*sizeof(struct inode));
    struct disk_block *temp_disk_blocks = malloc(temp_super_block.blocks*sizeof(struct disk_block));
    fread(temp_inodes, temp_super_block.inodes*sizeof(struct inode), 1, file);
    fread(temp_disk_blocks, temp_super_block.blocks*sizeof(struct disk_block), 1, file);
    printf("SUPERBLOCK\n");
    printf("\t inodes amount: %d\n\t blocks amount: %d\n", temp_super_block.inodes, temp_super_block.blocks);
    printf("\nINODES\n");
    for (size_t i = 0; i < temp_super_block.inodes; i++)
    {
        printf("%ld.\t name: %s | isdir: %d | next: %d\n",i , temp_inodes[i].name, temp_inodes[i].dir ,temp_inodes[i].next);
    }

    printf("\nBLOCKS\n");
    for (size_t i = 0; i < temp_super_block.blocks; i++)
    {
        printf("%ld.\t next: %d\n",i , temp_disk_blocks[i].next);
    }


    fclose(file);
    free(temp_disk_blocks);
    free(temp_inodes);
}

void lsfs(char* target){
    printdir("/root/");
}

void printdir(const char* pathname) {
    myDIR* dirp = myopendir(pathname);
    int fd = dirp->fd;
    if (inodes[fd].dir==0) {
        errno = 20;
	printf("Error attempting to list contents of file system.\n");
        return; //-1;
    }
    printf("NAME OF DIRECTORY: %s\n", inodes[fd].name);
    struct mydirent* currdir = (struct mydirent*)disk_blocks[inodes[fd].next].data;
    for (size_t i = 0; i < currdir->size; i++)
    {
	if (inodes[currdir->fds[i]].dir == 0){
          printf("\tfile number %ld: %s \n",i, inodes[currdir->fds[i]].name);
	}
	else {
	  printf("NAME OF DIRECTORY: %s\n", inodes[currdir->fds[i]].name);
	}
    }
    myclosedir(dirp);
    printf("\nDONE\n");
}

int mycreatefile(const char *path, const char* name) {
    char fin[256];
    strcpy(fin, "root");
    strcat(fin, path);
    printf("Path is %s\n", fin);
    int newfd = allocate_file(1, name);
    myDIR* dirp = myopendir(fin);//path;
    struct mydirent *currdir = myreaddir(dirp);
    currdir->fds[currdir->size++] = newfd;
    myclosedir(dirp);
    return newfd;
}

void printfd(int fd){
  
  /*  if(openfiles[fd].fd == -1)
    {
        errno = 13;
        return -1;
    }
    */
    int rb = inodes[fd].next;
    printf("NAME: %s\n", inodes[fd].name);

    while(rb!=-2) {
        if (rb==-1) {
            errno = 131;
	    printf("Error retrieving file\n");
            return;// -1;
        }
        printf("%s", disk_blocks[rb].data);

        rb = disk_blocks[rb].next;

    }
    printf("\nDONE %s\n", inodes[fd].name);
}

myDIR* myopendir(const char *pathname) {
  
    char str[80];
    strcpy(str, pathname);
    char *token;
    const char s[2] = "/";
    token = strtok(str, s);
    char currpath[NAME_SIZE] = "";
    char lastpath[NAME_SIZE] = "";
    while(token != NULL ) {
        if (token != NULL) {
            strcpy(lastpath, currpath);
            strcpy(currpath, token);
        }
        token = strtok(NULL, s);
    }
    for (size_t i = 0; i < super_block.inodes; i++)
    {
        if (!strcmp(inodes[i].name, currpath)) {
            if (inodes[i].dir!=1) {
                errno = 20;
		printf("Error attempting to open directory.\n");
                return NULL;//-1;
            }
            myDIR* newdir = (myDIR*)malloc(sizeof(myDIR));
            newdir->fd = i;
            return newdir;
        }
    }
    myDIR* newdir = (myDIR*)malloc(sizeof(myDIR));
    newdir->fd = mymkdir(lastpath, currpath);
    return newdir;
}

struct mydirent *myreaddir(myDIR* dirp) {
    /**
     * @brief Uses @param dirp to find the asked directory using fd, and @return it as a @struct mydirent.
     */
    int fd = dirp->fd;
    if (inodes[fd].dir!=1) {
        errno = 20;
	printf("Error attempting to read directory\n");
        return NULL;//-1;
    }
    return (struct mydirent*)disk_blocks[inodes[fd].next].data;
}

int myclosedir(myDIR* dirp) {
    /** Raise error as it was not implemented in this file system. */
    free(dirp);
}

int mymkdir(const char *path, const char* name) {
    /**
     * @brief This function goes through the path and finds the FD of the last directory in the path.
     * Then, it creates a new directory inside the FD that was found.
     */
    myDIR* dirp = myopendir(path);
    int fd = dirp->fd;
    if (fd==-1) {
        errno = 2;
        return -1;
    }
    if (inodes[fd].dir!=1) {
        errno = 20;
        return -1;
    }
    int dirblock = inodes[fd].next;
    struct mydirent* currdir = (struct mydirent*)disk_blocks[dirblock].data;
    if (currdir->size>=MAX_DIR_SIZE) {
        errno = 31;
        return -1;
    }
    int newdirfd = allocate_file(sizeof(struct mydirent), name);
    currdir->fds[currdir->size++] = newdirfd;
    inodes[newdirfd].dir = 1;
    struct mydirent* newdir = malloc(sizeof(struct mydirent));
    newdir->size = 0;
    for (size_t i = 0; i < MAX_DIR_SIZE; i++)
    {
        newdir->fds[i] = -1;
    }

    char *newdiraschar = (char*)newdir;

    for (size_t i = 0; i < sizeof(struct mydirent); i++)
    {
        writebyte(newdirfd, i, newdiraschar[i]);
    }
    strcpy(newdir->d_name, name);
    myclosedir(dirp);
    return newdirfd;
}

void addfilefs(char* fname){
    char *dirc, *basec, *bname, *dname;
    dirc = strdup(fname);
    basec = strdup(fname);
    bname = basename(basec);
    dname = dirname(dirc);
    char *newstring;
    printf("Adding directory: %s, file: %s\n", dname, bname);
    mycreatefile(dname, bname);
}


void removefilefs(char* fname){
    //int fd = myopen(fname, 0);
    printf("Removing file %s\n", fname);
    for (size_t i = 0; i < super_block.inodes; i++)
    {
        if (inodes[i].name == fname) {
	    inodes[i].next = -1;
	    printf("Removed file %s\n", inodes[i].name);
            return;
        }
    }
}


void extractfilefs(char* fname){
     //myDIR* dirp = myopendir(fname);
    //int fd = dirp->fd;
    int fd = myopen(fname, 0);
    if (fd == -1)
	    printf("Error retrieving file %s\n", fname);
    printf("fd to print is %d\n", fd);
    printfd(fd);
    myclose(fd);
}
