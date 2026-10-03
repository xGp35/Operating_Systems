#ifndef __vector_header_h__
#define __vector_header_h__

#define VECTOR_SIZE (100)

typedef struct __vector {
    pthread_mutex_t lock;
    int values[VECTOR_SIZE]; // Values is the actual array of the vector class. So if we define a vector *v (a pointer), to put values in the vector we put values in the "values" array. Like v->values
} vector_t;

// Quick rule of thumb: see a * in the parameter type? Use '->', No *? Use '.'
#endif // __vector_header_h__

