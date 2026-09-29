// Bounded Buffers
// In kernel Bounded Buffer - UNIX pipe

int buffer;
// Ususally this is a shared buffer, but we are just taking it as a global variable.
// Because the buffer and the global variable ablove share the property of being shared by
// both producer and consumer. (The int is even more shared as it can be used by anyone)
// Nevertheless, we digress, it has the important property of being shared by atleast 2 folks -
// The producer and the director, jk the consumer.

int count = 0;
void put (int value) {
    assert(count == 0); // Put assumes the buffer is empty, as the buffer capacity is 1.
    count = 1;
    buffer = value;
}

void get() {
    assert(count == 1); 
    count = 0;
    return buffer;
}

// Oh, producer produces, then the consumer consumes, ie set the value to 0
