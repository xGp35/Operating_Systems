// Case 1
void (*cherno)(void*);

// Say this is written independently as a line of code. This is a function pointer named cherno. This function pointer can point to any function that takes in one argument of type void * (void pointer) and returns void (no return)

// Case 2
void *(*cherno)(void*);

// Say this is written independently as a line of code. This is a function pointer named cherno. This function pointer can point to any function that takes in one argument of type void * (void pointer) and also returns void * (a void pointer, or a pointer that can then be typecasted to any specific pointer type if we know what that pointer would point to)

// Case 3
void *(*cherno)(void*);

// say this is present in the argument of a function. Like 
int lovelyFunc(void *(*cherno)(void*))
// Is this same as the case 2?


// Ans -
// Yes — case 3 is exactly the same type as case 2.

// Function parameter declarators use the identical syntax rules as ordinary variable declarators. When you write
int lovelyFunc(void *(*cherno)(void*))
// the parameter cherno is parsed with the exact same precedence you already worked out for case 2: *cherno says cherno is a pointer, 
// (*cherno)(void*) says what it's a pointer to — a function taking one void* — 
// and the leading void * says that function returns void*. 
// Putting it inside lovelyFunc(...) doesn't change any of that parsing; it just means lovelyFunc takes, as its argument, something of that function-pointer type





// Say i want to create a function named PoolBoy. This function will be the one to be passed to the pthread_create function. Now doe the return type of PoolBoy have to be void*?
// Can PoolBoy have the following signature?
int PoolBoy(int, int)          // -> No Completely wrong, more details below
// Or does it have to be
void * PoolBoy(void *)         // -> Yes, completely correct, more details below
// Is this allowed
char * PoolBoy(void *)         // -> Can run, but not good code
// Is this allowes
char * PoolBoy(char *)         // -> Can run, but not good code

// Ans -
//Case 1
int PoolBoy(int, int) — not allowed.
//Wrong parameter count, wrong parameter types, wrong return type. This isn't compatible with 
void *(*)(void*) 
// at all — the compiler will refuse to convert it (error, not just warning), and even a forced cast wouldn't help because pthread_create will call it as if it takes one void* and returns void*. The actual calling convention mismatch (arguments/return laid out differently) means this would genuinely crash or corrupt state, not just be "technically" UB.
//Case 2
void *PoolBoy(void *)
// yes, this is the exact required signature.
// This is what pthread_create's third parameter is declared as: void *(*start_routine)(void *). No cast needed, fully standard-conformant.
//Case 3
char *PoolBoy(void *)
// compiles, but only with a cast, and it's technically UB.
//The return type differs (char* vs void*), so this function's type is char *(*)(void*), not void *(*)(void*). If you pass it directly, the compiler will emit an incompatible-pointer-type warning (or error, depending on strictness). You can silence it with a cast:
pthread_create(&t, NULL, (void *(*)(void *))PoolBoy, arg);
//and on every real-world platform this works fine, 
// because char* and void* have identical representation, size, and calling convention. 
// But per the C standard (6.3.2.3), calling a function through a pointer whose type doesn't match the function's actual type is undefined behavior — it just happens to be UB that no real compiler/ABI punishes for plain data pointers. Not something to rely on; just declare it void* and cast inside the function body instead.
// Case 4
char *PoolBoy(char *)
// same deal, now on both ends.
// Both the parameter and return type differ from void*. Same fix: cast the function pointer when passing it to pthread_create. The arg you pass in (the 4th argument to pthread_create) is fine either way — void* converts to/from any object pointer type implicitly, that part isn't the issue. It's specifically the function pointer type that has to match void *(*)(void*), and only an explicit cast gets you there.