// fsync is used to write immediately to disk, ie, flush "dirty" data to disk immediately.
// fsync returns once all the data is written to disk

// Additionally we should also fsync the directory that contains the file foo.
// Adding this step ensures that not only the file itself is on disk, but also linking of this file to its directory is also done. (in case this was newly created file)

int fd = open("foo", O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
assert(fd > -1);
int rc = write(fd, buffer, size);
assert(rc == size); // we check if write actally wrote everything to disk, or did it write less bytes.
rc = fsync(fd);
assert(rc == 0);  // check fsync ran successfully or not. If it ran successfully its return code is 0. That is waht we assert here.

// write() returns how much ever byetes it wrote to disk.
// That is what we capture as the rc (return code) in this code

// buffer - its a inprocess private memory. Most probably stored in the heap