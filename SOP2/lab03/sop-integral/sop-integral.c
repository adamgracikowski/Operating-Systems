#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/mman.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define UNUSED(x) (void)(x)
#define NEXT_DOUBLE(a, b) (((double)rand() / RAND_MAX) * ((b) - (a)) + (a))
#define NEXT_INT(a, b) (rand() % ((b) - (a) + 1))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define SEM_NAME "/sop-integral-sem"
#define SHM_NAME "/sop-integral"
#define PERM 0666
#define INTEGRATING_PROCESS_COUNT 4
#define INTEGRATING_FUNCTIONS_COUNT 5
#define MIN_DOMAIN -20
#define MAX_DOMAIN 20
#define MIN_ITERATIONS 1e2
#define MAX_ITERATIONS 1e6

typedef double (*func_t)(double);
typedef struct range_t
{
    double a;
    double b;
} range_t;
typedef struct shm_data_t
{
    pthread_barrier_t integral_barrier;
    pthread_cond_t integral_cv;
    pthread_mutex_t integral_mtx;
    int turn;
    int integral_counter;
    range_t process_range[INTEGRATING_PROCESS_COUNT];
    int process_n[INTEGRATING_PROCESS_COUNT];
    double integral[INTEGRATING_FUNCTIONS_COUNT];
    range_t integral_range[INTEGRATING_FUNCTIONS_COUNT];
} shm_data_t;

double func1(double x) { return x; }
double func2(double x) { return x + 2; }
double func3(double x) { return 3 * x; }
double func4(double x) { return x * x; }
double func5(double x) { return -(x * x); }

func_t function[INTEGRATING_FUNCTIONS_COUNT] = {&func1, &func2, &func3, &func4, &func5};

void sop_barrierattr_shared(pthread_barrierattr_t *attr);
void sop_mutexattr_shared(pthread_mutexattr_t *attr);
void sop_condattr_shared(pthread_condattr_t *attr);
void sop_mutex_sharedlock(pthread_mutex_t *mtx);
void sop_mutex_sharedunlock(pthread_mutex_t *mtx);
shm_data_t *sop_shm_init(int *integrating_index);
void sop_shm_destroy(shm_data_t *shm_data);
double monte_carlo(double a, double b, func_t f, int n);
void display_integral(shm_data_t *shm_data, int function_index);

int main(void)
{
    srand((unsigned)time(NULL) * getpid());

    int integrating_index;
    shm_data_t *shm_data = sop_shm_init(&integrating_index);
    printf("[%d] My index is %d\n", getpid(), integrating_index);

    printf("[%d]: All %d integrating processes are ready!\n", getpid(), (int)INTEGRATING_PROCESS_COUNT);

    for (int i = 0; i < INTEGRATING_FUNCTIONS_COUNT; i++)
    {
        sop_mutex_sharedlock(&shm_data->integral_mtx);
        while (shm_data->turn != integrating_index)
            pthread_cond_wait(&shm_data->integral_cv, &shm_data->integral_mtx);

        shm_data->process_range[integrating_index].a =
            integrating_index == 0 ? 
            shm_data->integral_range[i].a : 
            shm_data->process_range[integrating_index - 1].b;
        shm_data->process_range[integrating_index].b =
            integrating_index == INTEGRATING_PROCESS_COUNT - 1 ? 
            shm_data->integral_range[i].b : 
            NEXT_DOUBLE(shm_data->process_range[integrating_index].a, shm_data->integral_range[i].b);

        shm_data->turn = (shm_data->turn + 1) % INTEGRATING_PROCESS_COUNT;
        pthread_cond_broadcast(&shm_data->integral_cv);
        sop_mutex_sharedunlock(&shm_data->integral_mtx);

        shm_data->process_n[integrating_index] = NEXT_INT((int)MIN_ITERATIONS, (int)MAX_ITERATIONS);
        shm_data->integral[integrating_index] = monte_carlo(
            shm_data->process_range[integrating_index].a,
            shm_data->process_range[integrating_index].b,
            function[i],
            shm_data->process_n[integrating_index]);

        printf("[%d]: Integral of func%d on [%lf, %lf] is: %lf\n",
               getpid(),
               i,
               shm_data->process_range[integrating_index].a,
               shm_data->process_range[integrating_index].b,
               shm_data->integral[integrating_index]);

        if (pthread_barrier_wait(&shm_data->integral_barrier) == PTHREAD_BARRIER_SERIAL_THREAD)
            display_integral(shm_data, i);
        pthread_barrier_wait(&shm_data->integral_barrier);
    }

    if (pthread_barrier_wait(&shm_data->integral_barrier) == PTHREAD_BARRIER_SERIAL_THREAD)
    {
        printf("[%d]: Destroyed shm objects!\n", getpid());
        printf("[%d]: Unmapped memory!\n", getpid());
        sop_shm_destroy(shm_data);
        munmap(shm_data, sizeof(shm_data_t));
        if (shm_unlink(SHM_NAME))
            ERR("shm_unlink");
        printf("[%d]: Unlinked %s!\n", getpid(), SHM_NAME);
        if (sem_unlink(SEM_NAME))
            ERR("sem_unlink");
        printf("[%d]: Unlinked %s!\n", getpid(), SEM_NAME);
    }
    else
    {
        printf("[%d]: Unmapped memory!\n", getpid());
        munmap(shm_data, sizeof(shm_data_t));
    }

    return EXIT_SUCCESS;
}

void sop_barrierattr_shared(pthread_barrierattr_t *attr)
{
    pthread_barrierattr_init(attr);
    pthread_barrierattr_setpshared(attr, PTHREAD_PROCESS_SHARED);
}

void sop_mutexattr_shared(pthread_mutexattr_t *attr)
{
    pthread_mutexattr_init(attr);
    pthread_mutexattr_setpshared(attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(attr, PTHREAD_MUTEX_ROBUST);
}

void sop_condattr_shared(pthread_condattr_t *attr)
{
    pthread_condattr_init(attr);
    pthread_condattr_setpshared(attr, PTHREAD_PROCESS_SHARED);
}

void sop_mutex_sharedlock(pthread_mutex_t *mtx)
{
    int ret;
    if ((ret = pthread_mutex_lock(mtx)))
    {
        if (ret == EOWNERDEAD)
            pthread_mutex_consistent(mtx);
        else
            ERR("pthread_mutex_lock");
    }
}

void sop_mutex_sharedunlock(pthread_mutex_t *mtx)
{
    pthread_mutex_unlock(mtx);
}

shm_data_t *sop_shm_init(int *integrating_index)
{
    sem_t *sem_ptr;
    if ((sem_ptr = sem_open(SEM_NAME, O_CREAT, PERM, 1)) == SEM_FAILED)
        ERR("sem_open");

    sem_wait(sem_ptr);

    shm_data_t *shm_data;

    errno = 0;
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, PERM);

    if (shm_fd >= 0)
    {
        if ((shm_data = mmap(NULL, sizeof(shm_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        {
            sem_post(sem_ptr);
            ERR("mmap");
        }
        if (ftruncate(shm_fd, sizeof(shm_data_t)))
        {
            shm_unlink(SHM_NAME);
            sem_post(sem_ptr);
            ERR("mmap");
        }

        close(shm_fd);

        memset(shm_data, 0, sizeof(shm_data_t));

        errno = 0;
        pthread_barrierattr_t barrier_attr;
        sop_barrierattr_shared(&barrier_attr);
        pthread_barrier_init(&shm_data->integral_barrier, &barrier_attr, INTEGRATING_PROCESS_COUNT);

        pthread_mutexattr_t mutex_attr;
        sop_mutexattr_shared(&mutex_attr);
        pthread_mutex_init(&shm_data->integral_mtx, &mutex_attr);

        pthread_condattr_t cond_attr;
        sop_condattr_shared(&cond_attr);
        pthread_cond_init(&shm_data->integral_cv, &cond_attr);

        if (errno)
        {
            shm_unlink(SHM_NAME);
            sem_post(sem_ptr);
            ERR("shmem init");
        }

        for (int i = 0; i < INTEGRATING_FUNCTIONS_COUNT; i++)
        {
            int a = NEXT_DOUBLE(MIN_DOMAIN, MAX_DOMAIN);
            int b = NEXT_DOUBLE(MIN_DOMAIN, MAX_DOMAIN);
            shm_data->integral_range[i].a = MIN(a, b);
            shm_data->integral_range[i].b = MAX(a, b);
            printf("[%d]: func%d on [%lf, %lf]\n", getpid(), i, shm_data->integral_range[i].a, shm_data->integral_range[i].b);
        }

        shm_data->integral_counter = 1;
        *integrating_index = 0;

        for (int i = 0; i < INTEGRATING_PROCESS_COUNT - 1; i++)
            sem_post(sem_ptr);
    }
    else if (errno == EEXIST)
    {
        if ((shm_fd = shm_open(SHM_NAME, O_RDWR, PERM)) < 0)
            ERR("shm_open");
        if ((shm_data = mmap(NULL, sizeof(shm_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
            ERR("mmap");

        close(shm_fd);

        sop_mutex_sharedlock(&shm_data->integral_mtx);
        *integrating_index = shm_data->integral_counter++;
        sop_mutex_sharedunlock(&shm_data->integral_mtx);
    }
    else
    {
        sem_post(sem_ptr);
        ERR("shm_open");
    }

    printf("[%d]: Waiting for integrating processes!\n", getpid());
    pthread_barrier_wait(&shm_data->integral_barrier);

    return shm_data;
}

void sop_shm_destroy(shm_data_t *shm_data)
{
    pthread_barrier_destroy(&shm_data->integral_barrier);
    pthread_mutex_destroy(&shm_data->integral_mtx);
    pthread_cond_destroy(&shm_data->integral_cv);
}

double monte_carlo(double a, double b, func_t f, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += f(NEXT_DOUBLE(a, b));
    return (sum / n) * (b - a);
}

void display_integral(shm_data_t *shm_data, int function_index)
{
    double sum = 0.0;
    for (int i = 0; i < INTEGRATING_PROCESS_COUNT; i++)
    {
        sum += shm_data->integral[i];
    }
    printf("[%d]: Final result of integral of func%d on [%lf, %lf] is: %lf\n",
           getpid(),
           function_index,
           shm_data->integral_range[function_index].a,
           shm_data->integral_range[function_index].b,
           sum);
}
