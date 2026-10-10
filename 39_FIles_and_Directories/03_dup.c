// dup() allows a process to create a new file descriptor that refers to the same underlying file as an existing descriptor. What about offset? - offset is also same. Both the file descriptors will point to the same entry in the open file table.

int main(int argc, char* argv[]) {
    int fd = open("README", O_RDONLY);
    assert(fd >= 0);
    int fd2 = dup(fd);
    // now fd and fd2 can be used interchangably
    return 0;
}