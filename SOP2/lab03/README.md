# Shared Memory:
### Functions:
```c
#include <sys/mman.h>

void* mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
/* Opis:
    - establish a mapping between the address space of the process at an address pa for len bytes to the memory object represented by the file descriptor fildes at offset off for len bytes.
    - the parameter prot determines whether read, write, execute, or some combination of accesses are permitted to the data being mapped.
    - the file descriptor fildes shall have been opened with read permission, regardless of the protection options specified. If PROT_WRITE is specified, the application shall ensure that it has opened the file descriptor fildes with write permission unless MAP_PRIVATE is specified.
    - the mapping type is retained across fork().
    - all implementations interpret an addr value of 0 as granting the implementation complete freedom in selecting pa. A non-zero value of addr is taken to be a suggestion of a process address near which the mapping should be placed.
    - on success returns the address at which the mapping was placed, otherwise it returns MAP_FAILED and sets errno to indicate the error.

Possible values of prot parameter:
- PROT_READ - Data can be read.
- PROT_WRITE - Data can be written.
- PROT_EXEC - Data can be executed.
- PROT_NONE - Data cannot be accessed.

Possible values of flags parameter:
- MAP_SHARED - Changes are shared (write references shall change the underlying object).
- MAP_PRIVATE - Changes are private (modifications to the mapped data by the calling process shall be visible only to the calling process and shall not change the underlying object).
- MAP_FIXED - Interpret addr exactly.
- MAP_ANONYMOUS - The mapping is not backed by any file and its contents are initialized to zero. The fd argument is ignored (but should be -1 for portability). The off parameter should be set to 0.

Common errors:
- EACCES - The fildes argument is not open for read, regardless of the protection specified, or fildes is not open for write and PROT_WRITE was specified for a MAP_SHARED type mapping.
- EBADF - The fildes argument is not a valid open file descriptor.
*/

int munmap(void *addr, size_t len);
/* Opis:
    - remove any mappings for those entire pages containing any part of the address space of the process starting at addr and continuing for len bytes.
    - further references to these pages shall result in the generation of a SIGSEGV signal to the process.
    - if there are no mappings in the specified address range, then munmap() has no effect.
    - on success returns 0, otherwise -1 and sets the errno to indicate the error.
*/

int msync(void *addr, size_t len, int flags);
/* Opis:
    - synchronizes memory with physical storage.
    - if no such storage exists, msync() need not have any effect.
    - on success returns 0, otherwise -1 and sets the errno to indicate the error.
        
Possible flags values:
- MS_ASYNC - Perform asynchronous writes.
- MS_SYNC - Perform synchronous writes (shall not return until all write operations are completed).
- MS_INVALIDATE - Invalidate cached data.
*/

#include <string.h>
void *memcpy(void* destination, const void* source, size_t n);
/* Opis:
    - copies n bytes from the object pointed to by source into the object pointed to by destination.
*/

void *memset(void *s, int c, size_t n);
/* Opis:
    - sets bytes in memory.
*/

#include <unistd.h>
int ftruncate(int fildes, off_t length);
/* Opis:
    - truncates a file to a specified length.
    - if the file size is increased, the extended area shall appear as if it were zero-filled.
    - if fildes refers to a shared memory object, ftruncate() shall set the size of the shared memory object to length.
    - on success returns 0, otherwise -1 and sets the errno to indicate the error.
*/

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
int shm_open(const char *name, int oflag, mode_t mode);
/* Opis:
    - creates and opens a new, or opens an existing, POSIX shared memory object.
    - a shared memory object should be identified by the name of the form: /somename.
    - returns a new file descriptor referring to the shared memory object.
    - on failure returns -1 and sets the errno to indicate the error.

Possible oflag values:
- O_RDONLY - open the object for read access (a shared object opened this way can only be mmaped for read access, that is with PROT_READ set).
- O_RDWR - open the object for read-write access.
- O_CREAT - create the shared memory object if it does not exist. A new shared memory object initially has zero length — the size of the object can be set using ftruncate. The newly allocated bytes of a shared memory object are automatically initialized to 0.
- O_TRUNC - if the shared memory object already exists, truncate it to zero bytes.
*/

int shm_unlink(const char *name);
/* Opis:
    - removes an object previously created by shm_open().
    - on failure returns -1 and sets the errno to indicate the error.
*/

/* Typical shared memory workflow:
1. Define a structure that will be imposed on the memory object that is shared between programs.
2. Use shm_open() to create shared memory object.
3. Use ftruncate() to size the object to match the size of your structure.
4. Use mmap() to map the object into the process's address space.
5. Initialize all the necessary components of your structure (semaphores, mutexes, etc.)
*/
```

# Synchronization:

## Semaphores:
### Functions:
```c
/* Basic Theory:
A semaphore is an integer whose value is never allowed to fall below zero. Two operations can be performed on semaphores: increment the semaphore value by one (sem_post) and decrement the semaphore value by one (sem_wait). If the value of a semaphore is currently zero, then a sem_wait() operation will block until the value becomes greater than zero.

POSIX semaphores come in two forms: named semaphores and unnamed semaphores.
- Named semaphore - is identified by a name of the form "/somename". Two processes can operate on the same named semaphore by passing the same name to sem_open(). When a process has finished using the semaphore, it can use sem_close() to close the semaphore. When all processes have finished using the semaphore, it can be removed from the system using sem_unlink(). On Linux, named semaphores are mounted under /dev/shm.
- Unnamed semaphores - it does not have a name. It is placed in a region od memory that is shared between multiple threads or processes. Before being used, an unnamed semaphore must be initialized using sem_init(). When it is no longer required it should be deallocated using sem_destroy().
*/

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
sem_t* sem_open(const char *name, int oflag);
sem_t* sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
/* Opis:
    - creates a new POSIX semaphore or opens an existing semaphore.
    - on success returns the address of the new semaphore, on failure returns SEM_FAILED and sets the errno to indicate the error.
*/

#include <semaphore.h>
int sem_init(sem_t *sem, int pshared, unsigned value);
/* Opis:
    - initializes the unnamed semaphore reffered to by sem.
    - if the pshared argument is != 0, then the semaphore is shared between processes
    - if the pshared argument == 0, then the semaphore is shared between threads of the process
    - on success returns 0, otherwise -1 and sets the errno to indicate the error.
*/

int sem_destroy(sem_t *sem);
/* Opis:
    - destroys an unnamed semaphore
    - the effect of subsequent use of the semaphore sem is undefined until sem is reinitialized by another call to sem_init()
    - on success returns 0, otherwise -1 and sets the errno to indicate the error.
*/

int sem_wait(sem_t *sem);
/* Opis:
    - locks a semaphore (decreases its internal value by one).
    - if the semaphore value is currently zero, then the calling thread shall not return from the call to sem_wait() until it either locks the semaphore or the call is interrupted by a signal.
    - returns 0 if the calling process successfully performed the semaphore lock operation.
    - if the call was unsuccessful, the state of the semaphore is unchanged, and the function returns a value of -1 and sets errno to indicate the error.
*/

int sem_trywait(sem_t *sem);
/* Opis:
    - locks the semaphore referenced by sem only if the semaphore is currently not locked (that is, if the semaphore value is currently positive).
    - otherwise, it doesn't lock the semaphore.
    - returns 0 if the calling process successfully performed the semaphore lock operation.
    - if the call was unsuccessful, the state of the semaphore is unchanged, and the function returns a value of -1 and sets errno to indicate the error
    
The sem_trywait() function may fail if:
- EAGAIN - the semaphore was already locked, so it cannot be immediately locked by the sem_trywait() operation.

The sem_trywait() and sem_wait() functions may fail if:
- EDEADLK - a deadlock condition was detected.
- EINTR - signal interrupted this function.
- EINVAL - the sem argument does not refer to a valid semaphore.
*/

#include <semaphore.h>
#include <time.h>
int sem_timedwait(sem_t *sem, const struct timespec *abstime);
/* Opis:
    - if the semaphore cannot be locked without waiting for another process or thread to unlock the semaphore by performing a sem_post() function, this wait terminates when the specified timeout expires.
    - the timeout is based on the CLOCK_REALTIME clock (clock_gettime() should be used).
    - on success returns 0, otherwise -1 and sets the errno to indicate the error.
    - ETIMEDOUT is set if the semaphore could not be locked before the specified timeout expired.
*/

int sem_post(sem_t *sem);
/* Opis:
    - unlocks a semaphore (increases its internal value by one).
    - if the semaphore value resulting from this operation is positive, then no threads were blocked waiting for the semaphore to become unlocked.
    - on success returns 0, otherwise -1 and sets the errno to indicate the error.
*/

int sem_getvalue(sem_t *sem, int *sval);
/* Opis:
    - places the current value of the semaphore pointed to sem into the integer pointed to by sval.
    - on success returns 0, otherwise -1 and sets the errno to indicate the error.
*/

#define _GNU_SOURCE
#include <pthread.h>
int pthread_setname_np(pthread_t thread, const char *name);
int pthread_getname_np(pthread_t thread, char* name, size_t size);
/* Opis:
    - sets or gets a unique name for a thread.
*/
```

## Mutexes:
### Functions:
```c
/*
Critical section - a section of code that accesses a shared resource and whose execution should be atomic (not interupted by another thread that simultaneously accesses the same shared resource).

Mutex - a mutual exclusion object which has two states: locked and unlocked. When a thread blocks a mutex, it becomes the owner of that mutex. Only the mutex owner can unlock it.

General accessing protocol:
1. Lock the mutex for the shared resource.
2. Access the shared resource (modify it).
3. Unlock the mutex.
*/

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

int pthread_mutex_unlock(pthread_mutex_t * mutex);

int pthread_mutex_lock(pthread_mutex_t * mutex);
/* Opis:
    - if the current mutex is unlocked, this call locks the mutex and returns immediately.
    - if the mutex is currently locked by another thread, then pthread_mutex_lock blocks until the mutex is unlocked, and then performs its action.
    - trying to block a mutex that is already owned by the thread results in EDEADLK.
    - if more than one threads is waiting for a mutex, it is indeterminate which thread will succeed in acquiring it.
*/

int pthread_mutex_trylock(pthread_mutex_t *mutex);
/* Opis: 
    - if the mutex object referenced by mutex is currently locked (by any thread, including the current thread), the call returns immediately.
    - if mutex type is PTHREAD_MUTEX_RECURSIVE, and the mutex is currently owned by the calling thread, the mutex lock count is incremented and pthread_mutex_trylock returns immediately.
    - on success returns 0, otherwise it may set EBUSY if the mutex could not be acquired because it was already locked.
*/

int pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec *abstime);
/* Opis: 
    - if the mutex cannot be locked without waiting for another thread to unlock the mutex, this wait shall be terminated when the specified timeout expires.
    - the timeout should be based on CLOCK_REALTIME (clock_gettime() should be used).
*/

/* Types of mutex:
- PTHREAD_MUTEX_NORMAL
- PTHREAD_MUTEX_ERRORCHECK
- PTHREAD_MUTEX_RECURSIVE
- PTHREAD_MUTEX_DEFAULT
*/

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
/* Opis:
    - sets the mutex type attribute.
*/

int pthread_mutexattr_gettype(const pthread_mutexattr_t* attr, int *restrict type);
/* Opis:
    - gets the mutex type attribute.
    - PTHREAD_MUTEX_NORMAL.
    - PTHREAD_MUTEX_ERRORCHECK.
    - PTHREAD_MUTEX_RECURSIVE.
    - PTHREAD_MUTEX_DEFAULT.
*/         

// Attributes:

int pthread_mutexattr_getpshared(const pthread_mutexattr_t* attr, int* pshared);
/* Opis:
    - places the value of the process-shared attribute of the mutex attributes object referred to by attr in the location pointed to by pshared.
    - on success returns 0, otherwise a positive error number.
*/

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
/* Opis:
    - sets the value of the process-shared attribute of the mutex attributes object referred to by attr to the value specified in pshared.
    - PTHREAD_PROCESS_PRIVATE - mutexes created with this attributes object are to be shared only among threads in the same process that initialized the mutex.
    - PTHREAD_PROCESS_SHARED - mutexes created with this attributes object can be shared between any threads that have access to the memory containing the object, including threads in different processes.
    - on success returns 0, otherwise a positive error number.
*/

int pthread_mutexattr_getrobust(const pthread_mutexattr_t* attr, int* robust);
/* Opis:
    - sets the mutex robust attribute.
*/

int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robust);
/* Opis:
    - sets the mutex robust attribute.
    - PTHREAD_MUTEX_STALLED - no special actions are taken if the owner of the mutex is terminated while holding the mutex lock.
    - PTHREAD_MUTEX_ROBUST - if the process containing the owning thread of a robust mutex terminates while holding the mutex lock, the next thread that acquires the mutex shall be notified about the termination by the return value EOWNERDEAD from the locking function.
    - The notified thread can then attempt to make the state protected by the mutex consistent again, and if successful can mark the mutex state as consistent by calling pthread_mutex_consistent().
    - if the mutex is unlocked without a call to pthread_mutex_consistent(), it shall be in a permanently unusable state and all attempts to lock the mutex shall fail with the error ENOTRECOVERABLE.
    - on success returns 0, otherwise a positive error number.
*/

int pthread_mutex_consistent(pthread_mutex_t *mutex);
/* Opis:
    - mark state protected by robust mutex as consistent.
    - on success returns 0, otherwise a positive error number.
*/
```

## Conditional Variable:
### Functions:
```c
#include <pthread.h>
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_getpshared(const pthread_condattr_t* attr, int* pshared);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
```

## Barriers:
### Functions:
```c
#include <pthread.h>
int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
int pthread_barrierattr_getpshared(const pthread_barrierattr_t* attr, int* pshared);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared);
```

## Przykłady:
```c
// Mapowanie całego pliku do pamięci procesu:
// 1. Otwieramy plik:
int fd;
if((fd = open(name, O_RDWR)) < 0)
    ERR("open");
// 2. Pobieramy rozmiar mapowanego pliku:
struct stat st;
if (fstat(fd, &st) < 0) 
    ERR("stat");
// 3. Wywołujemy mmap():
void *ptr= mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
if (ptr == MAP_FAILED)
    ERR("mmap");
// 4. Zamykamy otwarty plik:
if(close(fd))
    ERR("close")
// 5. Teraz możemy operować na pliku tak jak na tablicy na przykład:
memset(ptr, c, s.st_size);
// ... inne operacje
// 6. Na koniec wywołujemy unmap():
if(unmap(ptr, st.st_size) < 0)
    ERR("unmap");
    
// Tworzenie obiektu przy pomocy shm_open():
// 1. Na początku można wywołać dla bezpieczeństwa shm_unlink():
shm_unlink("/sop_shm");
// 2. Wywołujemy shm_open() określając flagi i uprawnienia:
int shm_fd = shm_open("/sop_shm", O_CREAT | O_EXCL | O_RDWR, 0600);
if (shm_fd < 0) ERR("shm_open")
// 3. Nadajemy utworzonemu obiektowi porządany rozmiar:
if (ftruncate(shm_fd, SHM_SIZE) < 0) 
    ERR("ftruncate");
// 4. Wywołujemy unmap() na utworzonym obiekcie shm:
void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
if (ptr == MAP_FAILED)
    ERR("munmap");
// 5. Zamykamy niepotrzebny deskryptor:
if(close(shm_fd)) ERR("close");
// 6. Teraz możemy wykonywać różne operacje na współdzielonej pamięci ...
// 7. Na koniec wywołujemy munmap():
if(munmap(ptr, SHM_SIZE))
    ERR("munmap");
    
// Tworzenie nazwanego semafora:
sem_t *sem = sem_open("/sop_sem", O_CREAT | O_EXCL, 0600, 1);
if (sem == SEM_FAILED) 
    ERR("sem_open");
sem_close(sem);
```

```c
// Bufor cykliczny z wykorzystaniem semaforów:
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10
#define ITERATIONS 10000000

sem_t mtx;
sem_t empty;
sem_t nonempty;

typedef struct bounded_buffer {
    int items[N];
    int in;
    int out;
} bounded_buffer_t;

void *producer(void *arg) {
    bounded_buffer_t *buffer = (bounded_buffer_t *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&empty);
        sem_wait(&mtx);

        int new_item = i;
        buffer->items[buffer->in] = new_item;
        buffer->in = (buffer->in + 1) % N;

        sem_post(&mtx);
        sem_post(&nonempty);
    }

    return NULL;
}

void *consumer(void *arg) {
    bounded_buffer_t *buffer = (bounded_buffer_t *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&nonempty);
        sem_wait(&mtx);

        int item = buffer->items[buffer->out];
        buffer->out = (buffer->out + 1) % N;

        if (item != i) {
            fprintf(stderr, "item corruption!\n");
        }

        sem_post(&mtx);
        sem_post(&empty);
    }

    return NULL;
}

int main() {

    bounded_buffer_t buffer = {};

    sem_init(&mtx, 0, 1);
    sem_init(&empty, 0, N);
    sem_init(&nonempty, 0, 0);

    srand(getpid());

    printf("Creating producer\n");

    pthread_t producer_tid, consumer_tid;
    int ret;
    if ((ret = pthread_create(&producer_tid, NULL, producer, &buffer)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Creating consumer\n");

    if ((ret = pthread_create(&consumer_tid, NULL, consumer, &buffer)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Created both threads\n");

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    return 0;
}
```

```c
// Problem wielu czytających i pojedynczego pisarza:
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10
#define ITERATIONS 10000000

int counter = 0;

sem_t mtx;
int readers_count;
sem_t rw_mtx;

void *writer(void *arg) {
    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&rw_mtx);

        printf("writing (%d)...\n", i);
        counter++;

        sem_post(&rw_mtx);
    }

    return NULL;
}

void *reader(void *arg) {
    for (int i = 0; i < ITERATIONS; ++i) {

        while (rand() % 100); // Slow down readers a bit

        sem_wait(&mtx);
        readers_count++;
        if (readers_count == 1) {
            sem_wait(&rw_mtx);
        }
        sem_post(&mtx);

        printf("reading (%lu)...\n", pthread_self());

        sem_wait(&mtx);
        readers_count--;
        if (readers_count == 0)
            sem_post(&rw_mtx);
        sem_post(&mtx);
    }

    return NULL;
}

int main() {

    srand(getpid());

    sem_init(&mtx, 0, 1);
    sem_init(&rw_mtx, 0, 1);

    printf("Creating writer\n");

    pthread_t writer_tid, reader1_tid, reader2_tid;
    int ret;
    if ((ret = pthread_create(&writer_tid, NULL, writer, NULL)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Creating reader 1\n");

    if ((ret = pthread_create(&reader1_tid, NULL, reader, NULL)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Creating reader 2\n");

    if ((ret = pthread_create(&reader2_tid, NULL, reader, NULL)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Created both threads\n");

    pthread_join(writer_tid, NULL);
    pthread_join(reader1_tid, NULL);
    pthread_join(reader2_tid, NULL);

    return 0;
}
```
```c
// Semafor w pamięci współdzielonej:
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>

#define BUFFER_SIZE 10
#define ITERATIONS 10000000

typedef struct context {
    sem_t mtx;
    sem_t empty;
    sem_t nonempty;

    int items[BUFFER_SIZE];
    int in;
    int out;
} context_t;

#define SHM_SIZE sizeof(context_t)

void producer(context_t *ctx) {
    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&ctx->empty);
        sem_wait(&ctx->mtx);

        int new_item = i;
        ctx->items[ctx->in] = new_item;
        ctx->in = (ctx->in + 1) % BUFFER_SIZE;

        sem_post(&ctx->mtx);
        sem_post(&ctx->nonempty);
    }
}

void consumer(context_t *ctx) {
    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&ctx->nonempty);
        sem_wait(&ctx->mtx);

        int item = ctx->items[ctx->out];
        ctx->out = (ctx->out + 1) % BUFFER_SIZE;

        if (item != i) {
            fprintf(stderr, "item corruption!\n");
        }

        sem_post(&ctx->mtx);
        sem_post(&ctx->empty);
    }
}


int main() {

    srand(getpid());

    shm_unlink("/sop_shm");

    int fd = shm_open("/sop_shm", O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        perror("shm_open()");
        return 1;
    }

    if (ftruncate(fd, SHM_SIZE) < 0) {
        perror("ftruncate()");
        return 1;
    }

    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap()");
        return 1;
    }

    close(fd);

    context_t *ctx = (context_t *) ptr;
    if (sem_init(&ctx->mtx, 1, 1) < 0) {
        perror("sem_init()");
        return 1;
    }
    if (sem_init(&ctx->empty, 1, BUFFER_SIZE) < 0) {
        perror("sem_init()");
        return 1;
    }
    if (sem_init(&ctx->nonempty, 1, 0) < 0) {
        perror("sem_init()");
        return 1;
    }

    switch (fork()) {
        case -1:
            perror("fork()");
            return 1;
        case 0: {
            // child
            consumer(ctx);
            break;
        }
        default: {
            // parent
            producer(ctx);
            while (wait(NULL) > 0);
            break;
        }
    }

    printf("Unmapping file\n");
    sem_destroy(&ctx->mtx);
    sem_destroy(&ctx->empty);
    sem_destroy(&ctx->nonempty);
    munmap(ptr, SHM_SIZE);
    return 0;
}

```
```c
// Bufor cykliczny z wykorzystaniem zmiennych warunkowych:
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>

#define BUFFER_SIZE 10
#define ITERATIONS 20

typedef struct context {
    pthread_mutex_t mtx;
    pthread_cond_t empty;
    pthread_cond_t nonempty;

    int items[BUFFER_SIZE];
    int in;
    int out;
    int size;
} context_t;

#define SHM_SIZE sizeof(context_t)

void producer(context_t *ctx) {
    for (int i = 0; i < ITERATIONS; ++i) {
        struct timespec ts = {0, (rand() % 1000) * 1000000};
        nanosleep(&ts, NULL);

        pthread_mutex_lock(&ctx->mtx);
        while (ctx->size >= BUFFER_SIZE)
            pthread_cond_wait(&ctx->empty, &ctx->mtx);

        int new_item = i;
        ctx->items[ctx->in] = new_item;
        ctx->in = (ctx->in + 1) % BUFFER_SIZE;
        ctx->size++;
        printf("produced '%d'\n", new_item);

      pthread_cond_signal(&ctx->nonempty);
      pthread_mutex_unlock(&ctx->mtx);
    }
}

void consumer(context_t *ctx) {
    for (int i = 0; i < ITERATIONS; ++i) {
        pthread_mutex_lock(&ctx->mtx);
        while (ctx->size == 0)
            pthread_cond_wait(&ctx->nonempty, &ctx->mtx);

        int item = ctx->items[ctx->out];
        ctx->out = (ctx->out + 1) % BUFFER_SIZE;
        ctx->size--;

        printf("consumed '%d'\n", item);

        pthread_cond_signal(&ctx->empty);
        pthread_mutex_unlock(&ctx->mtx);
    }
}


int main() {

    srand(getpid());

    shm_unlink("/sop_shm");

    int fd = shm_open("/sop_shm", O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        perror("shm_open()");
        return 1;
    }

    if (ftruncate(fd, SHM_SIZE) < 0) {
        perror("ftruncate()");
        return 1;
    }

    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap()");
        return 1;
    }

    close(fd);

    context_t *ctx = (context_t *) ptr;
    memset(ctx, 0, sizeof(context_t));

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);

    int err;
    printf("Creating mutex and CVs...\n");
    if ((err = pthread_mutex_init(&ctx->mtx, &mutex_attr)) != 0) {
        fprintf(stderr, "pthread_mutex_init(): %s", strerror(err));
        return 1;
    }
    if ((err = pthread_cond_init(&ctx->empty, &cond_attr)) != 0) {
        fprintf(stderr, "pthread_mutex_init(): %s", strerror(err));
        return 1;
    }
    if ((err = pthread_cond_init(&ctx->nonempty, &cond_attr)) != 0) {
        fprintf(stderr, "pthread_mutex_init(): %s", strerror(err));
        return 1;
    }

    switch (fork()) {
        case -1:
            perror("fork()");
            return 1;
        case 0: {
            // child
            consumer(ctx);
            break;
        }
        default: {
            // parent
            producer(ctx);
            while (wait(NULL) > 0);

            pthread_mutex_destroy(&ctx->mtx);
            pthread_cond_destroy(&ctx->empty);
            pthread_cond_destroy(&ctx->nonempty);
            break;
        }
    }

    printf("Unmapping shm\n");
    munmap(ptr, SHM_SIZE);
    return 0;
}

```
```c
// Obiadujący filozofowie:
#define _GNU_SOURCE
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#define N 3
#define ITERATIONS 10000000

typedef struct context {
  int idx;
  sem_t *left;
  sem_t *right;
} context_t;

void *philosopher(void *arg) {
  context_t *ctx = (context_t *)arg;

  for (int i = 0; i < ITERATIONS; ++i) {
    sem_wait(ctx->left);
    sem_wait(ctx->right);

    printf("Philosopher %d eats\n", ctx->idx);

    struct timespec ts = {0, 50000000000};
    nanosleep(&ts, NULL);

    sem_post(ctx->right);
    sem_post(ctx->left);

    printf("Philosopher %d thinks\n", ctx->idx);

    nanosleep(&ts, NULL);
  }

  return NULL;
}

int main() {
  int ret;
  sem_t chopsticks[N];
  pthread_t tids[N];
  context_t ctxs[N];

  for (int i = 0; i < N; ++i) {
    sem_init(&chopsticks[i], 0, 1);
    ctxs[i].idx = i;
    ctxs[i].left = &chopsticks[i];
    ctxs[i].right = &chopsticks[(i + 1) % N];
  }

  printf("Creating philosophers\n");

  for (int i = 0; i < N; ++i) {
    if ((ret = pthread_create(&tids[i], NULL, philosopher, &ctxs[i])) != 0) {
      fprintf(stderr, "pthread_create(): %s", strerror(ret));
      return 1;
    }
    char name[16];
    snprintf(name, sizeof(name), "philosopher %d", i);
    pthread_setname_np(tids[i], name);
  }

  printf("Joining all threads\n");

  for (int i = 0; i < N; ++i) {
    pthread_join(tids[i], NULL);
  }

  for (int i = 0; i < N; ++i) {
    sem_destroy(&chopsticks[i]);
  }

  return 0;
}

```
## Multiple readers, single writer locks:
```c
#include <pthread.h>

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
int pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t* rwlock, const struct timespec* abstime);
/* Opis:
    - applies a read lock to the read-write lock referenced by rwlock. The calling thread acquires the read lock if a writer does not hold the lock and there are no writers blocked on the lock.
    - if a signal is delivered to a thread waiting for a read-write lock for reading, upon return from the signal handler the thread resumes waiting for the read-write lock for reading as if it was not interrupted.
    - on success returns 0, otherwise a positive number to indicate the error.
    - the timeout in pthread_rwlock_timedrdlock shall be based on the CLOCK_REALTIME clock.
*/

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedwrlock(pthread_rwlock_t* rwlock, const struct timespec* abstime);
/* Opis:
    - applies a write lock to the read-write lock referenced by rwlock.
    - the calling thread shall acquire the write lock if no thread (reader or writer) holds the read-write lock rwlock.
    - otherwise, if another thread holds the read-write lock rwlock, the calling thread shall block until it can acquire the lock.
*/

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
/* Opis:
    - releases a lock held on the read-write lock object referenced by rwlock.
*/

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t* attr, int* pshared);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);
```