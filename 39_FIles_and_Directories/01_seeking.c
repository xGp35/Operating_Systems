off_t lseek(int fil_des, off_t offset, int whence);

// possible values of Whence  
SEEK_SET - offset is set to "offset"(the parameter given above) bytes
SEEK_CUR - offset is set to its current location plus offset bytes.
SEEK_END - offset is set to size of the file plus offset bytes. (Seems odd)

struct file {
    int ref;
    char readable;
    char writable;
    struct inode *ip;
    uint off;
}


file = open("file", O_RDONLY);
lseek(fd, 200, SEEK_SET);
read(fd, buffer, 50);
close(fd);

// The offset of each opened file is tracked by a kernel level table calle OPEN FILE TABLE